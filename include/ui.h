#ifndef UI_H
#define UI_H
#include <core.h>
#include <limits>
#include <raylib/raylib.h>

namespace ui {
  using namespace core;

  struct RGBA {
    u8 r{0};
    u8 g{0};
    u8 b{0};
    u8 a{0};
  };

  RGBA NewRGB(u8 red, u8 green, u8 blue);

  struct Vec2 {
    f32 x{0.0};
    f32 y{0.0};

    Vec2 operator*(const Vec2& other);
    Vec2 operator*(f32 other);
  };

  struct Rect {
    f32 x{0.0};
    f32 y{0.0};
    f32 w{0.0};
    f32 h{0.0};
  };

  enum class SizeKind {
    Pixels,
    Text,
    SumOfChildren,
    MaxOfChildren,
    PercentOfParent,
  };

  struct Size {
    SizeKind kind{SizeKind::Pixels};
    f32 value{0.0};
  };

  struct Stroke {
    RGBA color;
    f32 thickness{0.0};
  };

  struct WidgetId {
    u64 id{0};

    bool operator==(const WidgetId& other) const;
  };

  const WidgetId NO_ID = { std::numeric_limits<u64>::max() };

  struct Widget;

  struct WidgetTree {
    Widget* first_child{nullptr};
    Widget* last_child{nullptr};
    Widget* sibling{nullptr};
    Widget* parent{nullptr};
  };

  struct Layout {
    f32 computed_size[2] = {0.0, 0.0};
    Rect bounds;
    const char* text_string{""};
    Vec2 text_size;
  };

  const Font DEFAULT_FONT = GetFontDefault();

  struct Text {
    String8 content;
    Font font{0};
    u16 size{18};
    RGBA color;
  };

  struct Widget {
    WidgetId id;
    // Logical size
    Vec2 offset;
    Size logical_size[2];
    Vec2 growth_axis;
    // Paint
    RGBA fill;
    Stroke stroke;
    f32 rounding{0.0};
    // Text
    Text text;
    // Hierarchy
    WidgetTree tree;
    // Layout
    Layout layout;
  };

  struct Input {
    WidgetId hovered_id{NO_ID};
    bool click{false};
    bool hold{false};
  };
  
  enum class NumVar {
    SPACER_WIDTH,
    SPACER_HEIGHT,
    ITEM_WIDTH,
    ITEM_HEIGHT,
    ITEM_THICK,
    ITEM_ROUNDING,
    LIST_THICK,
    COUNT,
  };

  enum class ColorVar {
    ITEM_FILL,
    ITEM_STROKE,
    ITEM_STROKE_HIGHLIGHT,
    ITEM_STROKE_INTERACT,
    LIST_FILL,
    LIST_STROKE,
    COUNT,
  };

  enum class FontVar {
    DEFAULT_FONT,
    COUNT,
  };
  
  struct Style {
    f32 nums[(usize)NumVar::COUNT];
    RGBA colors[(usize)ColorVar::COUNT];
    Font fonts[(usize)FontVar::COUNT];
  };

  struct NumPair {
    NumVar var;
    f32 value;
  };

  struct ColorPair {
    ColorVar var;
    RGBA value;
  };

  struct FontPair {
    FontVar var;
    Font value;
  };

  struct UiCtx {
    Arena arena{nullptr};
    Array<Widget> widgets;
    Style style;
    Input input;
  };

  UiCtx NewCtx(usize num_widgets = 1024);
  void Destroy(UiCtx* ctx);

  struct Ui {
    UiCtx* ctx{nullptr};
    Arena* arena{nullptr};
    Widget* active_parent{nullptr};
    Array<Widget*> widgets;
    Style style;
    Array<NumPair> num_stack;
    Array<ColorPair> color_stack;
    Array<FontPair> font_stack;
  };

  Ui* BeginUi(UiCtx* ctx, Arena* arena, Rect bounds);
  void EndUi(Ui* ui);

  // UI variable management
  void PushNumVar(Ui* ui, NumVar var, f32 value);
  void PopNumVar(Ui* ui);

  void PushColorVar(Ui* ui, ColorVar var, RGBA value);
  void PopColorVar(Ui* ui);

  void PushFontVar(Ui* ui, FontVar var, Font value);
  void PopFontVar(Ui* ui);

  // Widgets
  Widget* AddWidget(Ui* ui, WidgetId id = {0});

  void PopParent(Ui* ui);

  enum class SpaceKind {
    InLine,
    CrossLine,
  };
  void Label(Ui* ui, String8 text);

  void Space(Ui* ui, SpaceKind kind = SpaceKind::InLine, f32 multiplier = 1.0);
  bool Button(Ui* ui, String8 text);
  void Label(Ui* ui, String8 text);
  void Header(Ui* ui, String8 text);

  void VList(Ui* ui);
  void HList(Ui* ui);
}

#endif
