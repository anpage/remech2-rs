#[derive(Clone, Debug, Default)]
pub struct OverlayMouseState {
    pub pos_x: f32,
    pub pos_y: f32,
    pub left_down: bool,
    pub right_down: bool,
    pub middle_down: bool,
}
