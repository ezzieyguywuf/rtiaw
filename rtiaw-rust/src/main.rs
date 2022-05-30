use softbuffer::GraphicsContext;
use winit::{
    dpi::PhysicalSize,
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

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

// Per the Softbuffer requirements[1], each pixel is represented by 32 bits
// [1]: https://docs.rs/softbuffer/latest/softbuffer/struct.GraphicsContext.html#method.set_buffer
struct Framebuffer {
    pixels: Vec<u32>,
}

impl Framebuffer {
    fn new(width: usize, height: usize) -> Framebuffer {
        let background = Color {
            red: 175 as u8,
            green: 175 as u8,
            blue: 230 as u8,
        };

        let mut buffer = Framebuffer { pixels: Vec::new() };

        buffer.pixels = Vec::with_capacity(width * height);
        buffer.pixels.resize(width * height, background.to_bytes());

        buffer
    }
}

const WINDOW_HEIGHT: u32 = 711;
const ASECT_RATIO: f32 = 16.0 / 9.0;
const WINDOW_WIDTH: u32 = (WINDOW_HEIGHT as f32 * ASECT_RATIO) as u32;

fn main() {
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_resizable(false)
        .with_inner_size(PhysicalSize::new(WINDOW_WIDTH, WINDOW_HEIGHT))
        .build(&event_loop)
        .unwrap();
    let mut graphics_context = unsafe { GraphicsContext::new(window) }.unwrap();
    let buffer = Framebuffer::new(WINDOW_WIDTH as usize, WINDOW_HEIGHT as usize);

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::RedrawRequested(window_id) if window_id == graphics_context.window().id() => {
                graphics_context.set_buffer(
                    &buffer.pixels,
                    WINDOW_WIDTH as u16,
                    WINDOW_HEIGHT as u16,
                );
            }
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                window_id,
            } if window_id == graphics_context.window().id() => {
                *control_flow = ControlFlow::Exit;
            }
            _ => {}
        }
    });
}
