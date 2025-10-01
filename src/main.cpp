#include "ui.h"
#include <core.h>
#include <ostream>
#include <raylib/raylib.h>
#include <iostream>

using namespace core;

static inline
void BuildUi(ui::Ui* ui) {
    {
      ui::PushColorVar(ui, ui::ColorVar::LIST_FILL, ui->style.colors[(usize)ui::ColorVar::ITEM_FILL]);
      ui::PushNumVar(ui,ui::NumVar::LIST_THICK, 4.0);

      ui::VList(ui);

      ui::PopColorVar(ui);
      ui::PopNumVar(ui);

      ui::Space(ui);

      ui::Header(ui, Lit("Window"));

      ui::Space(ui);
  
      {
        ui::HList(ui);

        ui::Space(ui);

        if (ui::Button(ui, Lit("Test 1"))) {
          std::cout << "Clicked" << std::endl;
        }

        ui::Space(ui);

        if (ui::Button(ui, Lit("Test 2"))) {
          std::cout << "Clicked" << std::endl;
        }

        ui::Space(ui);

        ui::PopParent(ui);

      }

      ui::Space(ui);

      ui::PopParent(ui);
    }
}

MAKE_SLOTMAP_KEY(Entity);

struct EntityData {
  String8 name;
};

template <typename T>
u64 ToRawId(T id) {
  static_assert(sizeof(Entity) == 8);
  return *(u64*)(&id);
}

template <typename T>
T FromRawId(u64 raw) {
  static_assert(sizeof(T) == 8);
  return *(T*)(&raw);
}


int main() {
  InitWindow(1600, 900, "Test");
  SetTargetFPS(60);

  SlotMap<Entity, EntityData> entities{0};

  auto e1 = Insert(&entities, { .name = Lit("Federico") });
  auto e2 = Insert(&entities, { .name = Lit("Tianqi") });
  std::cout << "Name #1 " << Get(&entities, e1)->name << std::endl;
  std::cout << "Name #2 " << Get(&entities, e2)->name << std::endl;
  {
    auto it = Iter(&entities);
    auto num = 0;
    while (auto entry = Next(&it)) {
      assert(FromRawId<Entity>(ToRawId(entry.key)) == entry.key);
      
      std::cout << "#" << num << " " << entry.key.index << ", " << entry.key.generation << " " << entry.value->name << ";" << std::endl;
      num++;
    }
  }
  
  ui::UiCtx ui_ctx = ui::NewCtx();
  defer(Destroy(&ui_ctx));

  Arena frame_arena;
  defer(Free(&frame_arena));

  ui_ctx.style.fonts[(usize)ui::FontVar::DEFAULT_FONT] = LoadFontEx("assets/fonts/default.ttf", 28, nullptr, 0);

  while (!WindowShouldClose()) {

    if (IsKeyPressed(KEY_ESCAPE)) {
      break;
    }

    BeginDrawing();
    BeginBlendMode(BLEND_ALPHA);
    ClearBackground(RAYWHITE);

    auto ui = ui::BeginUi(&ui_ctx, &frame_arena, { 20.0, 20.0, 1600.0, 900.0 });
    BuildUi(ui);    
    ui::EndUi(ui);

    EndBlendMode();
    EndDrawing();

    Reset(&frame_arena);
  }

  CloseWindow();
  return 0;
}
