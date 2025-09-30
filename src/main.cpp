#include "ui.h"
#include <core.h>
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

        ui::Button(ui, Lit("Test 2"));

        ui::Space(ui);

        ui::PopParent(ui);

      }

      ui::Space(ui);

      ui::PopParent(ui);
    }
    
    ui::Space(ui);
}

int main() {
  InitWindow(1600, 900, "Test");
  SetTargetFPS(60);

  std::cout << Lit("Hello") << std::endl;

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
