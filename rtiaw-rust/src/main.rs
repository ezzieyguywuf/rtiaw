use softbuffer::GraphicsContext;
use winit::{
    dpi::PhysicalSize,
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

fn main() {
    // Define some constants
    let window_height: u32 = 711;
    let aspect_ratio: f32 = 16.0 / 9.0;
    let window_width: u32 = (window_height as f32 * aspect_ratio) as u32;

    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_resizable(false)
        .with_inner_size(PhysicalSize::new(window_width, window_height))
        .build(&event_loop)
        .unwrap();
    let mut graphics_context = unsafe { GraphicsContext::new(window) }.unwrap();

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::RedrawRequested(window_id) if window_id == graphics_context.window().id() => {
                let buffer = (0..((window_width * window_height) as usize))
                    .map(|index| {
                        let y = index / (window_width as usize);
                        let x = index % (window_width as usize);
                        let red = x % 255;
                        let green = y % 255;
                        let blue = (x * y) % 255;

                        let color = blue | (green << 8) | (red << 16);

                        color as u32
                    })
                    .collect::<Vec<_>>();

                graphics_context.set_buffer(&buffer, window_width as u16, window_height as u16);
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
