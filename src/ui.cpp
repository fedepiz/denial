#include "raylib/raylib.h"
#include <core.h>
#include <ui.h>
namespace ui {
  using namespace core;

  bool WidgetId::operator==(const WidgetId& other) const {
    return this->id == other.id;
  }

  UiCtx NewCtx(usize num_widgets) {
    // Create a new arena that will hold the UICtx's data
    Arena arena = NewArena(sizeof(Widget) * num_widgets);
    auto widgets = NewEmptyArray<Widget>(&arena, num_widgets);
    return {
      .arena = arena,
      .widgets = NewEmptyArray<Widget>(&arena, num_widgets),
    };
  }

  void Destroy(UiCtx* ctx) {
    Free(&ctx->arena);
  }

  RGBA NewRGB(u8 red, u8 green, u8 blue) {
    return { red, green, blue, 255 };
  }

  static inline
  Vec2 Corner(Rect rect) {
    return { rect.x, rect.y };
  }

  static inline
  Vector2 ToRay(Vec2 xy) {
    return { xy.x, xy.y };
  }

  static inline
  Rectangle ToRay(Rect rect) {
    return { rect.x, rect.y, rect.w, rect.h };
  }

  static inline
  Color ToRay(RGBA color) {
    return { color.r, color.g, color.b, color.a };
  }

  static inline
  Vec2 FromRay(Vector2 xy) {
    return { xy.x, xy.y };
  }


  static inline
  bool Contains(Rect rect, Vec2 point) {
    return
      point.x >= rect.x &&
      point.x <= rect.x + rect.w &&
      point.y >= rect.y &&
      point.y <= rect.y + rect.h;
  }

  static inline
  Size PixelSize(f32 value) {
    return { .kind = SizeKind::Pixels, .value = value };
  }

  struct SizePair {
    Size size[2];
  };
  
  static inline
  void SetPixelSize(Size size[2], Vec2 xy) {
    size[0] = PixelSize(xy.x);
    size[1] = PixelSize(xy.y);
  }

  inline static
  Style DefaultStyle() {
    Style style;
    NumPair num_pairs[] = {
      { NumVar::SPACER_WIDTH, 20.0 },
      { NumVar::SPACER_HEIGHT, 20.0 },
      { NumVar::ITEM_WIDTH, 80.0 },
      { NumVar::ITEM_HEIGHT, 30.0 },
      { NumVar::ITEM_THICK, 4.0 },
      { NumVar::ITEM_ROUNDING, 0.5 },
    };

    for (auto pair : num_pairs) {
      style.nums[(usize)pair.var] = pair.value;
    }

    ColorPair color_pairs[] = {
      { ColorVar::ITEM_FILL, NewRGB(150, 150, 100) },
      { ColorVar::ITEM_STROKE, NewRGB(0, 0, 0) },
      { ColorVar::ITEM_STROKE_HIGHLIGHT, NewRGB(255, 255, 0) },
      { ColorVar::ITEM_STROKE_INTERACT, NewRGB(0, 255, 0) }
    };

    for (auto pair : color_pairs) {
      style.colors[(usize)pair.var] = pair.value;
    }

    FontPair font_pairs[] = {
      { FontVar::ITEM_FONT, GetFontDefault() },
    };

    for (auto pair : font_pairs) {
      style.fonts[(usize)pair.var] = pair.value;
    }

    return style;
  }
  
  Ui* BeginUi(UiCtx* ctx, Arena* arena, Rect bounds) {
    // Clear the current widgets
    Clear(ctx->widgets);

    // Allocate he ui in the provided arena
    auto ui = Alloc<Ui>(arena);

    // Initialize
    *ui = {
      .arena = arena,
      .ctx = ctx,
      .active_parent = nullptr,
      .widgets = NewEmptyArray<Widget*>(arena, ctx->widgets->capacity),
      .style = DefaultStyle(),
      .num_stack = NewEmptyArray<NumPair>(arena, 20),
      .color_stack = NewEmptyArray<ColorPair>(arena, 20),
      .font_stack = NewEmptyArray<FontPair>(arena, 10),
    };

    // Create the first, root widget
    auto root = Emplace(ctx->widgets, {});
    assert(root);
    root->offset = Corner(bounds);
    SetPixelSize(root->logical_size, { bounds.w, bounds.h });
    root->growth_axis.y = 1.0;
    Push(ui->widgets, root);

    ui->active_parent = root;
    
    return ui;
  }

  enum class ReductionOp {
    Sum,
    Max,
  };
  
  inline static
  f32 ReduceChildrenComputedSize(Widget* parent, usize axis, ReductionOp op) {
    f32 accum = 0;
    auto child = parent->tree.first_child;
    while(child) {
      auto value = child->layout.computed_size[axis];          
      switch (op) {
        case ReductionOp::Sum:
          accum += value;
          break;
        case ReductionOp::Max:
          accum = Max(accum, value);
          break;
      }
      child = child->tree.sibling;
    }
    return accum;
  }

  inline static
  void Layout(Ui* ui) {
    // Reset all layouts
    for (auto widget : ui->widgets) {
      widget->layout = {0};
    }

    // Process text
    for (auto widget : ui->widgets) {
      auto text = widget->text;
      // Skip empty text
      if (IsEmpty(text.content)) {
        continue;
      }
      // Convert text to string
      widget->layout.text_string = CStr(ui->arena, text.content);
      // Cache text size
      auto measure = MeasureTextEx(text.font, widget->layout.text_string, text.size, 1);
      widget->layout.text_size = FromRay(measure);
    }
    
    for (int axis = 0; axis < 2; ++axis) {
      // Self-contained sizes
      for (auto widget : ui->widgets) {
        auto logical_size = widget->logical_size[axis];
        switch (logical_size.kind) {
          case SizeKind::Pixels:
            widget->layout.computed_size[axis] = logical_size.value;
            break;
          default:
            continue;
        }
      }

      // Child-dependent sizes
      for (int i = ui->widgets->len - 1; i >= 0; --i) {
        auto widget = Get(ui->widgets, i);
        auto logical_size = widget->logical_size[axis];

        ReductionOp op{ReductionOp::Sum};
        switch (logical_size.kind) {
          case SizeKind::SumOfChildren:
            op = ReductionOp::Sum;
            break;
            case SizeKind::MaxOfChildren:
            op = ReductionOp::Max;
            break;
          default:
            continue;
        }
        widget->layout.computed_size[axis] = ReduceChildrenComputedSize(widget, axis, op);
      }
    }

    // Set all sizes
    for (auto widget : ui->widgets) {
      auto* bounds = &widget->layout.bounds;
      bounds->w = widget->layout.computed_size[0];
      bounds->h = widget->layout.computed_size[1];
    }

    // Placement
    for (auto widget : ui->widgets) {
      // Offset the widget
      widget->layout.bounds.x += widget->offset.x;
      widget->layout.bounds.y += widget->offset.y;
      // Create cursor for children placement
      auto cursor = Corner(widget->layout.bounds);
      auto child = widget->tree.first_child;
      while (child) {
        child->layout.bounds.x = cursor.x;
        child->layout.bounds.y = cursor.y;
        cursor.x +=  child->layout.bounds.w * widget->growth_axis.x;
        cursor.y += child->layout.bounds.h * widget->growth_axis.y;
        child = child->tree.sibling;
      }
    }
  }

  static inline
  void ProcessInput(Ui* ui) {
    auto mouse_pos = FromRay(GetMousePosition());
    // Find top widget that is hovered
    Input input;
    input.click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    input.hold = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    for (int idx = ui->widgets->len - 1; idx >= 0; --idx) {
      auto widget = Get(ui->widgets, idx);
      if (Contains(widget->layout.bounds, mouse_pos)) {
        input.hovered_id = widget->id;
        break;
      }
    }
    ui->ctx->input = input;
  }

  static inline
  void Draw(Ui* ui) {
    for (auto widget : ui->widgets) {
      auto bounds = ToRay(widget->layout.bounds);
      const auto NUM_SEGMENTS = 4;
      
      if (widget->fill.a != 0) {
        auto color = ToRay(widget->fill);
        if (widget->rounding <= 0.0) {
          DrawRectangle(bounds.x, bounds.y, bounds.width, bounds.height, color);
        } else {
          DrawRectangleRounded(bounds, widget->rounding, NUM_SEGMENTS, color);
        }
      }

      auto thickness = widget->stroke.thickness;
      if (thickness > 0 && widget->stroke.color.a != 0) {
        auto color = ToRay(widget->stroke.color);
        if (widget->rounding <= 0.0) {
          DrawRectangleLinesEx(bounds, thickness, color);
        } else {
          DrawRectangleRoundedLinesEx(bounds, widget->rounding, NUM_SEGMENTS, thickness, color);
        }
      }

      auto text = widget->text;
      if (!IsEmpty(text.content)) {
        auto bounds = widget->layout.bounds;
        auto pos = Corner(bounds);
        pos.x += (bounds.w - widget->layout.text_size.x)/2.0;
        pos.y += (bounds.h - widget->layout.text_size.y)/2.0;
        auto color = ToRay(text.color);
        DrawTextEx(text.font, widget->layout.text_string, ToRay(pos), text.size, 1, color);
      }
    }
  }

  void EndUi(Ui* ui) {
    Layout(ui);    

    ProcessInput(ui);
    
    Draw(ui);
  }

  static inline
  void PushChild(Widget* parent, Widget* child) {
    if(!parent->tree.last_child) {
      parent->tree.first_child = child;
    } else {
      parent->tree.last_child->tree.sibling = child;
    }
    parent->tree.last_child = child;
    child->tree.parent = parent;
  }

  void PushNumVar(Ui* ui, NumVar var, f32 value) {
    auto current = ui->style.nums[(usize)var];
    bool pushed = Push(ui->num_stack, { var, current });
    assert(pushed);
    ui->style.nums[(usize)var] = value;
  }

  void PopNumVar(Ui* ui) {
    auto prev = Pop(ui->num_stack);
    ui->style.nums[(usize)prev.var] = prev.value;
  }

  void PushColorVar(Ui* ui, ColorVar var, RGBA value) {
    auto current = ui->style.colors[(usize)var];
    bool pushed = Push(ui->color_stack, { var, current });
    assert(pushed);
    ui->style.colors[(usize)var] = value;
  }

  void PopColorVar(Ui* ui) {
    auto prev = Pop(ui->color_stack);
    ui->style.colors[(usize)prev.var] = prev.value;
  }

  void PushFontVar(Ui* ui, FontVar var, Font value) {
    auto current = ui->style.fonts[(usize)var];
    bool pushed = Push(ui->font_stack, { var, current });
    assert(pushed);
    ui->style.fonts[(usize)var] = value;
  }

  void PopFontVar(Ui* ui) {
    auto prev = Pop(ui->font_stack);
    ui->style.fonts[(usize)prev.var] = prev.value;
  }

  struct Interaction {
    bool is_hovered{false};
    bool is_clicked{false};
    bool is_held{false};
  };

  static inline
  Interaction InteractionFor(Ui* ui, WidgetId id) {
    bool is_hovered = ui->ctx->input.hovered_id == id;
    bool is_clicked = is_hovered && ui->ctx->input.click;
    bool is_held = is_hovered && ui->ctx->input.hold;
    return {
      .is_hovered = is_hovered,
      .is_clicked = is_clicked,
      .is_held = is_held
    };
  }

  Widget* AddWidget(Ui* ui, WidgetId id) {
    auto widget = Emplace(ui->ctx->widgets, {0});
    assert(widget);

    widget->id = id;

    bool added = Push(ui->widgets, widget);
    assert(added);

    PushChild(ui->active_parent, widget);

    return widget;
  }

  void VSpace(Ui* ui) {
    auto widget = AddWidget(ui);
    auto size = ui->style.nums[(usize)NumVar::SPACER_HEIGHT];
    SetPixelSize(widget->logical_size, { 0.0, size });
  }

  void HSpace(Ui* ui) {
    auto widget = AddWidget(ui);
    auto size = ui->style.nums[(usize)NumVar::SPACER_WIDTH];
    SetPixelSize(widget->logical_size, { size, 0.0 });
  }

  static inline
  Text WidgetText(Ui* ui, String8 text) {
    auto font = ui->style.fonts[(usize)FontVar::ITEM_FONT];
    return {
      .content = text,
      .color = NewRGB(0, 0, 0),
      .font = font,
      .size = (u16)font.baseSize,
    };
  }

  bool Button(Ui* ui, String8 text) {
    WidgetId id = { Hash(SubstringUntil(text, '#')) };
    auto interaction = InteractionFor(ui, id);

    auto widget = AddWidget(ui, id);

    widget->text = WidgetText(ui, text);
    
    auto* st_nums = ui->style.nums;
    auto* st_colors = ui->style.colors;

    widget->fill = st_colors[(usize)ColorVar::ITEM_FILL];

    auto stroke_color = ColorVar::ITEM_STROKE;

    if (interaction.is_held) {
      stroke_color = ColorVar::ITEM_STROKE_INTERACT;
    } else if (interaction.is_hovered) {
      stroke_color = ColorVar::ITEM_STROKE_HIGHLIGHT;
    }

    widget->rounding = st_nums[(usize)NumVar::ITEM_ROUNDING];
    
    widget->stroke = { st_colors[(usize)stroke_color], st_nums[(usize)NumVar::ITEM_THICK] };
    SetPixelSize(widget->logical_size, { st_nums[(usize)NumVar::ITEM_WIDTH], st_nums[(usize)NumVar::ITEM_HEIGHT] });

    return interaction.is_clicked;
  }
}
