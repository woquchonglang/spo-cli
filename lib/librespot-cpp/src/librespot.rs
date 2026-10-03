use librespot::{
    connect::{ConnectConfig, LoadRequest, LoadRequestOptions, Spirc},
    core::{
        Error, authentication::Credentials, cache::Cache, config::SessionConfig, session::Session,
    },
    playback::mixer::MixerConfig,
    playback::{
        audio_backend,
        audio_backend::{Sink, SinkResult},
        config::{AudioFormat, PlayerConfig},
        convert::Converter,
        decoder::AudioPacket,
        mixer,
        player::Player,
    },
};

use librespot_core::SpotifyId;
use librespot_metadata;

use std::os::raw::{c_double, c_int};
use std::sync::OnceLock;
use tokio;
use tokio::runtime::Runtime;

pub struct SpotifyContext {
    session: Session,
}

impl SpotifyContext {
    pub fn new(session: Session) -> Self {
        Self { session }
    }
}

pub struct Lyrics {
    pub lines: Vec<(chrono::Duration, String)>,
}

pub type CavaSinkCallback =
    extern "C" fn(samples: *const c_double, per_channel: c_int, channels: c_int);

static CAVA_CALLBACK: OnceLock<CavaSinkCallback> = OnceLock::new();

#[unsafe(no_mangle)]
pub extern "C" fn cava_bridge_register(cb: CavaSinkCallback) {
    let _ = CAVA_CALLBACK.set(cb);
}

pub struct CavaSink;

impl Sink for CavaSink {
    fn start(&mut self) -> SinkResult<()> {
        Ok(())
    }
    fn stop(&mut self) -> SinkResult<()> {
        Ok(())
    }

    fn write(&mut self, packet: AudioPacket, _: &mut Converter) -> SinkResult<()> {
        if let AudioPacket::Samples(samples) = packet {
            if let Some(cb) = CAVA_CALLBACK.get() {
                let per_channel = (samples.len() / 2) as c_int;
                cb(samples.as_ptr(), per_channel, 2);
            }
        }
        Ok(())
    }
}

impl TeeSink {
    pub fn new(playback: Box<dyn Sink>, cava: CavaSink) -> Self {
        Self { playback, cava }
    }
}

impl Sink for TeeSink {
    fn start(&mut self) -> SinkResult<()> {
        self.cava.start()?;
        self.playback.start()
    }

    fn stop(&mut self) -> SinkResult<()> {
        self.cava.stop()?;
        self.playback.stop()
    }

    fn write(&mut self, packet: AudioPacket, converter: &mut Converter) -> SinkResult<()> {
        if let AudioPacket::Samples(samples) = &packet {
            if let Some(cb) = CAVA_CALLBACK.get() {
                let per_channel = (samples.len() / 2) as c_int;
                cb(samples.as_ptr(), per_channel, 2);
            }
        }
        self.playback.write(packet, converter)
    }
}

pub struct TeeSink {
    playback: Box<dyn Sink>,
    cava: CavaSink,
}

#[cxx::bridge]
mod ffi {

    #[namespace = "librespot"]
    struct LyricLine {
        time_ms: i64,
        text: String,
    }

    #[namespace = "librespot"]
    extern "Rust" {
        type SpotifyContext;

        fn play_backends() -> Box<SpotifyContext>;
        fn cancel_play_backends();
        fn get_lyrics_lines(ctx: &SpotifyContext, id: &str) -> Result<Vec<LyricLine>>;
        fn get_num_channels() -> u8;
        fn get_sample_rate() -> u32;
    }
}

impl From<librespot_metadata::lyrics::Lyrics> for Lyrics {
    fn from(value: librespot_metadata::lyrics::Lyrics) -> Self {
        let mut lines = value
            .lyrics
            .lines
            .into_iter()
            .map(|l| {
                let ms = l.start_time_ms.parse::<i64>().expect("invalid number");
                (chrono::Duration::milliseconds(ms), l.words.clone())
            })
            .collect::<Vec<_>>();
        lines.sort_by_key(|l| l.0);
        Self { lines }
    }
}

const CACHE: &str = ".cache";
const CACHE_FILES: &str = ".cache/files";

fn runtime() -> &'static Runtime {
    static RT: OnceLock<Runtime> = OnceLock::new();
    RT.get_or_init(|| Runtime::new().expect("Failed to create Tokio runtime"))
}

static CANCEL: tokio::sync::Notify = tokio::sync::Notify::const_new();
fn play_backends() -> Box<SpotifyContext> {
    runtime()
        .block_on(async {
            let session_config = SessionConfig::default();
            let player_config = PlayerConfig::default();
            let audio_format = AudioFormat::default();
            let connect_config = ConnectConfig::default();
            let mixer_config = MixerConfig::default();
            let request_options = LoadRequestOptions::default();

            let sink_builder = audio_backend::find(None).unwrap();
            let mixer_builder = mixer::find(None).unwrap();

            let cache =
                Cache::new(Some(CACHE), Some(CACHE), Some(CACHE_FILES), None).expect("cache error");
            let credentials = cache
                .credentials()
                .ok_or(Error::unavailable("credentials not cached"))
                .or_else(|_| {
                    librespot_oauth::OAuthClientBuilder::new(
                        &session_config.client_id,
                        "http://127.0.0.1:8898/login",
                        vec!["streaming"],
                    )
                    .open_in_browser()
                    .build()?
                    .get_access_token()
                    .map(|t| Credentials::with_access_token(t.access_token))
                })
                .expect("credentials error");

            let session = Session::new(session_config, Some(cache));
            let mixer = mixer_builder(mixer_config).expect("mixer error");
            mixer.set_volume(65535);

            let player = Player::new(
                player_config,
                session.clone(),
                mixer.get_soft_volume(),
                move || {
                    let playback = sink_builder(None, audio_format);
                    let cava = CavaSink;
                    Box::new(TeeSink::new(playback, cava)) as Box<dyn Sink>
                },
            );

            let (spirc, spirc_task) =
                Spirc::new(connect_config, session.clone(), credentials, player, mixer)
                    .await
                    .expect("spirc error");

            // these calls can be seen as "queued"
            // spirc.activate().expect("spirc activate error");
            // spirc
            //     .load(LoadRequest::from_context_uri(
            //         format!("spotify:user:{}:collection", session.username()),
            //         request_options,
            //     ))
            //     .expect("spirc load error");
            // spirc.play().expect("spirc play error");

            // starting the connect device and processing the previously "queued" calls
            tokio::spawn(async move {
                let _spirc = spirc;
                tokio::select! {
                    _ = spirc_task => {
                        eprintln!("spirc task ended");
                    }
                    _ = CANCEL.notified() => {
                        eprintln!("cancel signal received, stopping spirc");
                    }
                }
            });

            Ok::<Box<SpotifyContext>, String>(Box::new(SpotifyContext { session }))
        })
        .expect("play_backends failed")
}

fn get_lyrics_lines(ctx: &SpotifyContext, id: &str) -> Result<Vec<ffi::LyricLine>, String> {
    runtime().block_on(async {
        let spotify_id = SpotifyId::from_base62(id).map_err(|e| format!("Invalid ID: {e}"))?;

        let raw = futures::executor::block_on(librespot_metadata::lyrics::Lyrics::get(
            &ctx.session,
            &spotify_id,
        ))
        .map_err(|e| format!("Error: {e}"))?;

        let local: Lyrics = raw.into();

        Ok(local
            .lines
            .into_iter()
            .map(|(t, text)| ffi::LyricLine {
                time_ms: t.num_milliseconds(),
                text,
            })
            .collect())
    })
}

fn cancel_play_backends() {
    CANCEL.notify_one();
}

fn get_num_channels() -> u8 {
    librespot::playback::NUM_CHANNELS
}

fn get_sample_rate() -> u32 {
    librespot::playback::SAMPLE_RATE
}
