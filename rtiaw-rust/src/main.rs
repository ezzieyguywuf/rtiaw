use rand::prelude::*;
use softbuffer::GraphicsContext;
use std::error::Error;
use std::sync::mpsc;
use std::thread;
use winit::{
    dpi::PhysicalSize,
    event::{ElementState, Event, VirtualKeyCode, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

const WINDOW_HEIGHT: u32 = 711;
const ASECT_RATIO: f32 = 16.0 / 9.0;
const WINDOW_WIDTH: u32 = (WINDOW_HEIGHT as f32 * ASECT_RATIO) as u32;

fn main() -> Result<(), Box<dyn Error>> {
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_resizable(false)
        .with_inner_size(PhysicalSize::new(WINDOW_WIDTH, WINDOW_HEIGHT))
        .build(&event_loop)?;
    let mut graphics_context = unsafe { GraphicsContext::new(window) }?;
    let mut buffer = Framebuffer::new(WINDOW_WIDTH as usize, WINDOW_HEIGHT as usize);

    let (rx, _) = launch_threads()?;

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match rx.try_recv() {
            Ok(pixels) => {
                for pixel in pixels {
                    buffer.set(pixel);
                }
            }
            Err(_) => {}
        }
        match event {
            Event::RedrawRequested(window_id) if window_id == graphics_context.window().id() => {
                graphics_context.set_buffer(
                    &buffer.pixel_bytes,
                    WINDOW_WIDTH as u16,
                    WINDOW_HEIGHT as u16,
                );
            }
            Event::MainEventsCleared => {
                graphics_context.window().request_redraw();
            }
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                window_id,
            } if window_id == graphics_context.window().id() => {
                *control_flow = ControlFlow::Exit;
            }
            Event::WindowEvent {
                event:
                    WindowEvent::KeyboardInput {
                        device_id: _,
                        input,
                        is_synthetic: _,
                    },
                window_id: _,
            } if input.state == ElementState::Pressed
                && input.virtual_keycode == Some(VirtualKeyCode::Q) =>
            {
                *control_flow = ControlFlow::Exit;
            }
            _ => {}
        }
    });
}

struct Color {
    red: u8,
    green: u8,
    blue: u8,
}

impl Color {
    fn to_bytes(&self) -> u32 {
        self.blue as u32 | ((self.green as u32) << 8) | ((self.red as u32) << 16)
    }
}

// Top-left is row = 0, col = 0
struct Position {
    row: usize,
    col: usize,
}

struct Pixel {
    color: Color,
    location: Position,
}

// Per the Softbuffer requirements[1], each pixel is represented by 32 bits
// [1]: https://docs.rs/softbuffer/latest/softbuffer/struct.GraphicsContext.html#method.set_buffer
struct Framebuffer {
    pixel_bytes: Vec<u32>,
    width: usize,
}

impl Framebuffer {
    fn new(width: usize, height: usize) -> Framebuffer {
        let background = Color {
            red: 175 as u8,
            green: 175 as u8,
            blue: 230 as u8,
        };

        let mut buffer = Framebuffer {
            pixel_bytes: Vec::new(),
            width,
        };

        buffer.pixel_bytes = Vec::with_capacity(width * height);
        buffer
            .pixel_bytes
            .resize(width * height, background.to_bytes());

        buffer
    }

    fn set(&mut self, pixel: Pixel) {
        let i = pixel.location.row * self.width + pixel.location.col;
        self.pixel_bytes[i] = pixel.color.to_bytes();
    }
}

fn launch_threads(
) -> Result<(mpsc::Receiver<Vec<Pixel>>, Vec<thread::JoinHandle<()>>), Box<dyn Error>> {
    let n_thread = match std::thread::available_parallelism()?.get() {
        1 => 1,
        x => x - 1,
    };
    let (tx, rx) = mpsc::channel();
    let chunk_size: usize = WINDOW_WIDTH as usize / n_thread;

    let mut children = Vec::new();

    for n in 0..n_thread {
        let tx_clone = tx.clone();
        let lower: usize = n * chunk_size;
        let upper = if n == n_thread - 1 {
            WINDOW_WIDTH as usize - 1
        } else {
            lower + chunk_size
        };

        let child = thread::spawn(move || {
            let mut rng = rand::thread_rng();
            let red: u8 = rng.gen_range(0..255);
            let green: u8 = rng.gen_range(0..255);
            let blue: u8 = rng.gen_range(0..255);
            for row in 0..WINDOW_HEIGHT - 1 {
                let mut pixels = Vec::new();
                for col in lower..upper {
                    let pixel = Pixel {
                        color: Color { red, green, blue },
                        location: Position {
                            row: row as usize,
                            col: col as usize,
                        },
                    };
                    pixels.push(pixel);
                }
                tx_clone.send(pixels).unwrap();
            }
        });
        children.push(child);
    }

    Ok((rx, children))
}
