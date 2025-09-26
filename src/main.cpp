#include "ui.h"
#include <core.h>
#include <raylib/raylib.h>
#include <iostream>

using namespace core;

static inline
void BuildUi(ui::Ui* ui) {
    if (ui::Button(ui, Lit("Btn1"))) {
      std::cout << "Button pressed" << std::endl;
    }

    ui::VSpace(ui);

    ui::PushColorVar(ui, ui::ColorVar::ITEM_STROKE, ui::NewRGB(255, 0, 0));
    ui::Button(ui, Lit("Btn2"));
    ui::PopColorVar(ui);

    ui::VSpace(ui);

    ui::PushColorVar(ui, ui::ColorVar::ITEM_FILL, ui::NewRGB(50, 200, 122));
    ui::Label(ui, Lit("Hello goodbye, hello goodbye!"));
    ui::PopColorVar(ui);

    ui::VSpace(ui);

    ui::Button(ui, Lit("Btn3"));
}

int main() {
  InitWindow(1600, 900, "Test");
  SetTargetFPS(60);

  std::cout << Lit("Hello") << std::endl;

  ui::UiCtx ui_ctx = ui::NewCtx();
  defer(Destroy(&ui_ctx));

  Arena frame_arena;
  defer(Free(&frame_arena));

  Font default_font = LoadFontEx("assets/fonts/default.ttf", 28, nullptr, 0);

  ui::FontPair fonts[] = {
    { ui::FontVar::ITEM_FONT, default_font },
  };

  while (!WindowShouldClose()) {

    if (IsKeyPressed(KEY_ESCAPE)) {
      break;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    auto ui = ui::BeginUi(&ui_ctx, &frame_arena, { 20.0, 20.0, 1600.0, 900.0 });
    for (auto pair : fonts) {
      ui->style.fonts[(usize)pair.var] = pair.value;
    }
    BuildUi(ui);    
    ui::EndUi(ui);

    EndDrawing();

    Reset(&frame_arena);
  }

  CloseWindow();
  return 0;
}
