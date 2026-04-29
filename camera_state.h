#ifndef CAMERA_STATE_H
#define CAMERA_STATE_H

struct CameraState {
    // Position
    float pos_x         = 13.0f;
    float pos_y         =  2.0f;
    float pos_z         =  3.0f;
    // Look-at target
    float target_x      =  0.0f;
    float target_y      =  0.0f;
    float target_z      =  0.0f;
    // Lens
    float vfov          = 20.0f;
    float focus_dist    = 10.0f;
    float defocus_angle =  0.6f;
    // Quality
    int   samples_per_pixel = 100;
    int   max_depth         =  50;
};

namespace presets {

inline CameraState default_view() { return {}; }

inline CameraState wide_angle() {
    CameraState s;
    s.vfov          = 75.0f;
    s.defocus_angle =  0.0f;
    return s;
}

inline CameraState close_up() {
    CameraState s;
    s.pos_x         =  5.0f;
    s.pos_y         =  1.0f;
    s.pos_z         =  1.5f;
    s.vfov          = 25.0f;
    s.focus_dist    =  4.5f;
    s.defocus_angle =  2.0f;
    return s;
}

inline CameraState overhead() {
    CameraState s;
    s.pos_x         =  0.0f;
    s.pos_y         = 18.0f;
    s.pos_z         =  0.1f;
    s.vfov          = 50.0f;
    s.focus_dist    = 18.0f;
    s.defocus_angle =  0.0f;
    return s;
}

} // namespace presets

#endif
