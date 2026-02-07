use log;
use nalgebra::{Vector2, Vector4};
use rand::Rng;
use std::collections::VecDeque;
use surge_core::{
    app::SurgeApp,
    vulkan::sprite_database::{InstanceInfo, SpriteDatabase},
};
use winit::{
    event::{DeviceId, ElementState, KeyEvent, MouseButton},
    keyboard::{KeyCode, PhysicalKey},
};

const PIECE_Z: f32 = 0.0;
const BOARD_Z: f32 = 0.5;

const PIECE_DIMS: f32 = 105.0;
const SLOT_DIMS: f32 = 121.0;

const X_SLOT_BASE: f32 = 15.0;
const X_SLOT_TO_POS: [f32; 4] = [
    X_SLOT_BASE + 0.0 * SLOT_DIMS,
    X_SLOT_BASE + 1.0 * SLOT_DIMS,
    X_SLOT_BASE + 2.0 * SLOT_DIMS,
    X_SLOT_BASE + 3.0 * SLOT_DIMS,
];

const Y_SLOT_BASE: f32 = 315.0;
const Y_SLOT_TO_POS: [f32; 4] = [
    Y_SLOT_BASE + 0.0 * SLOT_DIMS,
    Y_SLOT_BASE + 1.0 * SLOT_DIMS,
    Y_SLOT_BASE + 2.0 * SLOT_DIMS,
    Y_SLOT_BASE + 3.0 * SLOT_DIMS,
];

const IMAGE_ASSET_PATHS: [&'static str; 12] = [
    "surge-modules/surge-mod-2048/assets/board.png",
    "surge-modules/surge-mod-2048/assets/pieces_2.png",
    "surge-modules/surge-mod-2048/assets/pieces_4.png",
    "surge-modules/surge-mod-2048/assets/pieces_8.png",
    "surge-modules/surge-mod-2048/assets/pieces_16.png",
    "surge-modules/surge-mod-2048/assets/pieces_32.png",
    "surge-modules/surge-mod-2048/assets/pieces_64.png",
    "surge-modules/surge-mod-2048/assets/pieces_128.png",
    "surge-modules/surge-mod-2048/assets/pieces_256.png",
    "surge-modules/surge-mod-2048/assets/pieces_512.png",
    "surge-modules/surge-mod-2048/assets/pieces_1024.png",
    "surge-modules/surge-mod-2048/assets/pieces_2048.png",
];

#[derive(Debug, PartialEq)]
enum MergeDirection {
    Up,
    Down,
    Left,
    Right,
}

#[derive(Debug, PartialEq)]
enum BoardState {
    Idle,
    Merge(MergeDirection),
    Resolve,
}

#[derive(Debug, PartialEq)]
struct Piece {
    pub value: usize,
    pub x: usize,
    pub y: usize,
}

impl Piece {
    fn new_random() -> Self {
        let mut rng = rand::rng();
        let value = rng.random_range(1..=2);
        let x = rng.random_range(0..4);
        let y = rng.random_range(0..4);
        Piece { value, x, y }
    }

    fn get_score(&self) -> usize {
        usize::pow(2, self.value as u32)
    }
}

pub struct App2048 {
    // Sequence of states to execute
    board_states: VecDeque<BoardState>,

    // Live pieces on the board
    live_pieces: Vec<Piece>,
}

impl App2048 {
    fn new() -> Self {
        let mut board_states = VecDeque::new();
        board_states.push_back(BoardState::Idle);

        let piece_a = Piece::new_random();
        let mut piece_b = Piece::new_random();

        while piece_a == piece_b {
            piece_b = Piece::new_random();
        }

        let mut live_pieces = Vec::new();
        live_pieces.push(piece_a);
        live_pieces.push(piece_b);

        App2048 {
            board_states,
            live_pieces,
        }
    }

    fn board_idle(&self) -> bool {
        match self.board_states.front() {
            Some(s) => *s == BoardState::Idle,
            None => true,
        }
    }

    fn add_live_pieces_sprites(&self, spdb: &mut SpriteDatabase) {
        for piece in &self.live_pieces {
            let piece_ii = InstanceInfo {
                position: Vector2::new(X_SLOT_TO_POS[piece.x], Y_SLOT_TO_POS[piece.y]),
                scale: Vector2::from_element(PIECE_DIMS),
                z: PIECE_Z,
                texture_id: piece.value,
                color_multiplier: Vector4::from_element(1.0),
                subtexture_info: None,
            };

            spdb.add_instance(&piece_ii);
        }
    }
}

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(App2048::new())
}

fn load_image_assets(spdb: &mut SpriteDatabase) {
    for asset_path in IMAGE_ASSET_PATHS {
        spdb.upload_texture(asset_path).unwrap();
    }
}

fn add_board_sprite(spdb: &mut SpriteDatabase) {
    let scale = nalgebra::Vector2::new(
        spdb.get_create_info().window_width,
        spdb.get_create_info().window_height,
    );

    let board_ii = InstanceInfo {
        position: Vector2::from_element(0.0),
        scale,
        z: BOARD_Z,
        texture_id: 0,
        color_multiplier: Vector4::from_element(1.0),
        subtexture_info: None,
    };

    spdb.add_instance(&board_ii);
}

impl SurgeApp for App2048 {
    fn on_load(&mut self, spdb: &mut SpriteDatabase) {
        surge_core::cli::init_env_logger();
        log::info!("2048 startup");
        load_image_assets(spdb);
    }

    fn on_swapchain_recreate(&self, spdb: &mut SpriteDatabase) {
        load_image_assets(spdb);
    }

    fn update(&mut self, _: f32, spdb: &mut SpriteDatabase) {
        add_board_sprite(spdb);
        self.add_live_pieces_sprites(spdb);

        let current_state = match self.board_states.front() {
            Some(s) => s,
            None => {
                self.board_states.push_back(BoardState::Idle);
                return;
            }
        };

        match current_state {
            BoardState::Idle => return,
            BoardState::Merge(merge_direction) => {
                log::info!("Board merge {:?}", merge_direction);
                self.board_states.pop_front();
            }
            BoardState::Resolve => {
                log::info!("Board resolve");
                self.board_states.pop_front();
            }
        }
    }

    fn keyboard_event(&mut self, _: DeviceId, event: KeyEvent, _: bool) {
        match event {
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowUp),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    log::info!("Move up");
                    self.board_states
                        .push_back(BoardState::Merge(MergeDirection::Up));
                    self.board_states.push_back(BoardState::Resolve);
                    self.board_states.push_back(BoardState::Idle);
                    self.board_states.pop_front();
                }
            }
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowDown),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    log::info!("Move Down");
                    self.board_states
                        .push_back(BoardState::Merge(MergeDirection::Down));
                    self.board_states.push_back(BoardState::Resolve);
                    self.board_states.push_back(BoardState::Idle);
                    self.board_states.pop_front();
                }
            }
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowLeft),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    log::info!("Move Down");
                    self.board_states
                        .push_back(BoardState::Merge(MergeDirection::Left));
                    self.board_states.push_back(BoardState::Resolve);
                    self.board_states.push_back(BoardState::Idle);
                    self.board_states.pop_front();
                }
            }
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowRight),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    log::info!("Move Down");
                    self.board_states
                        .push_back(BoardState::Merge(MergeDirection::Right));
                    self.board_states.push_back(BoardState::Resolve);
                    self.board_states.push_back(BoardState::Idle);
                    self.board_states.pop_front();
                }
            }
            _ => return,
        }
    }

    fn mouse_button_event(
        &mut self,
        device_id: DeviceId,
        state: ElementState,
        button: MouseButton,
    ) {
        log::info!(
            "Mouse event: ID = {:?} event = {:?} button = {:?}",
            device_id,
            state,
            button
        );
    }
}
