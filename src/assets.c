
// hold colors here as well
const Color SHOP_BG = { 250, 247, 220, 255 };
const Color SHOP_BG_ALT = { 242, 235, 190, 255 };
const Color SHOP_SHADOW = { 184, 174, 139, 255 };

const Color SHOP_ORANGE = { 255, 139, 35, 255 };
const Color SHOP_ORANGE_LIGHT = { 255, 185, 75, 255 };
const Color SHOP_ORANGE_DARK = { 220, 92, 12, 255 };

const Color SHOP_BLUE = { 28, 180, 220, 255 };
const Color SHOP_GREEN = { 34, 183, 92, 255 };
const Color SHOP_RED = { 225, 32, 48, 255 };
const Color SHOP_PINK = { 245, 35, 190, 255 };
const Color SHOP_YELLOW = { 255, 210, 60, 255 };

const Color SHOP_INK = { 73, 61, 44, 255 };
const Color SHOP_TEXT_MUTED = { 135, 122, 92, 255 };
const Color SHOP_WHITE = { 255, 253, 242, 255 };

// textures
Texture2D LOGO;
Texture2D ACCOUNT_SETTINGS_HOME_ICON;
Texture2D CHAIRS_HOME_ICON;
Texture2D OTHER_FURNISHINGS_HOME_ICON;
Texture2D LARGER_ITEMS_HOME_ICON;

void init_textures() {
    LOGO = LoadTexture("./assets/PEAK_Software_Logo.png");
    ACCOUNT_SETTINGS_HOME_ICON = LoadTexture("./assets/icons/account_settings_home_icon.png");
    CHAIRS_HOME_ICON = LoadTexture("./assets/icons/chairs_home_icon.png");
    OTHER_FURNISHINGS_HOME_ICON = LoadTexture("./assets/icons/other_furnishings_home_icon.png");
    LARGER_ITEMS_HOME_ICON = LoadTexture("./assets/icons/larger_items_home_icon.png");
}

Item_Resource* get_item_resource(Shop* shop, Item item) {
    Item_Resource* resource = ht_find(&shop->item_resources, item.id);
    if (resource) {
        return resource;
    }
    Item_Resource new_resource = {0};
    // try png
    const char* png_path = TextFormat(
        "assets/items/%s.png",
        item.name
    );
    if (FileExists(png_path)) {
        new_resource.texture = LoadTexture(png_path);
        new_resource.is_model = false;
        new_resource.scale = 1.5;
        *ht_put(&shop->item_resources, item.id) = new_resource;
        return ht_find(&shop->item_resources, item.id);
    }

    // try glb
    const char* glb_path = TextFormat(
        "assets/items/%s.glb",
        item.name
    );
    if (FileExists(glb_path)) {
        new_resource.model = LoadModel(glb_path);
        new_resource.is_model = true;
        BoundingBox box = GetModelBoundingBox(new_resource.model);
        float width  = box.max.x - box.min.x;
        float height = box.max.y - box.min.y;
        float depth  = box.max.z - box.min.z;
        float max_size = fmaxf(width, fmaxf(height, depth));
        new_resource.scale = 1.5 / max_size;
        *ht_put(&shop->item_resources, item.id) = new_resource;
        return ht_find(&shop->item_resources, item.id);
    }

    printf("no asset found for `%s`\n", item.name);
    return NULL;
}
