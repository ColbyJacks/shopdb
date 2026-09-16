
#define DISPLAY_Y 2.5

void init_shop_items(Shop* shop) {
    shop->camera.position = (Vector3){ 0.0f, DISPLAY_Y, 7.0f };
    shop->camera.target = (Vector3){ 0.0f, DISPLAY_Y, 0.0f };
    shop->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    shop->camera.fovy = 45.0f;
    shop->camera.projection = CAMERA_PERSPECTIVE;
}

void update_item_display(Shop* shop) {
    float wheel = GetMouseWheelMove();
    shop->scroll_target -= wheel * 2.0;
    if (IsKeyPressed(KEY_RIGHT)) {
        shop->scroll_target += 4.0;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        shop->scroll_target -= 4.0;
    }
    float max_scroll = (shop->item_count-1) * 4.0;
    shop->scroll_target = Clamp(shop->scroll_target, 0.0, max_scroll);
    float dt = GetFrameTime();
    shop->scroll = Lerp(shop->scroll, shop->scroll_target, 1.0-powf(0.001, GetFrameTime()));
}

void draw_item_display(Shop* shop) {
    Camera3D camera = shop->camera;
    camera.position.x = shop->scroll;
    camera.target.x = shop->scroll;

    BeginMode3D(camera);
    float spacing = 4.0;

    for (int i=0; i<shop->item_count; i++) {
        Item* item = &shop->items[i];
        float item_x = i * spacing;
        float center_dist = fabsf(item_x - shop->scroll);
        float focus = 1.0 - Clamp(center_dist / spacing, 0.0, 1.0);
        float scale = Lerp(0.65, 1.15, focus);

        Vector3 pos = { 
            item_x, 
            DISPLAY_Y + Lerp(-0.25, 0.0, focus), 
            Lerp(1.5, 0.0, focus) 
        };
        Vector3 size = Vector3Scale(item->size, scale);
        DrawCubeV(pos, size, WHITE);
        DrawCubeWiresV(pos, size, BLACK);
    }
    EndMode3D();
}
