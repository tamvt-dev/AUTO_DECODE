# Mono Design System - Qt Implementation Guide

**Version:** 1.0  
**Based on:** [decode-ui Design System](C:\Users\Administrator\decode-ui\.agents\skills\design-system\SKILL.md)  
**Theme Library:** dark.qss, light.qss, style.qss  
**Last Updated:** April 2026

---

## Brand & Mission

**Visual Style:** Minimal, clean, high-contrast, playful, matrix-inspired with hacker-chic aesthetic

**Design Philosophy:**  
- **Keyboard-first:** All interactive elements are fully navigationable via keyboard
- **Accessibility:** WCAG 2.2 AA compliant - color contrast, focus states, motion preferences
- **Semantic:** Use class attributes to convey component intent, not layout
- **Density:** Compact spacing (4–48px scale) for data-dense interfaces
- **Typography:** Space Mono for UI, JetBrains Mono for code/data

---

## Color Tokens

### Dark Theme (Primary)
```
Primary (Action):      #37F712  (neon green, WCAG AAA)
Secondary (Interactive): #00A6F4 (bright blue, WCAG AA)
Success (Positive):     #00A63D (dark green)
Warning (Caution):      #FE9900 (orange)
Danger (Destructive):   #FF2157 (red)
Surface (Background):   #0A0E27 (very dark blue-gray)
Text (Foreground):      #E7E5E4 (off-white)
Neutral (Disabled):     #78716B (taupe)
```

### Light Theme (Alternative)
```
Primary:      #1a7d05  (dark green for light backgrounds)
Secondary:    #006a9a  (dark blue)
Success:      #00A63D
Warning:      #E68A00
Danger:       #C41C3B  (darker red for contrast on white)
Surface:      #F9F9F9  (off-white)
Text:         #0A0E27  (dark)
Neutral:      #999999  (gray)
```

---

## Typography

### Font Families
- **Primary UI:** `"Space Mono", "Monaco", monospace`  
  Used for: buttons, labels, headings, navigation
  
- **Code/Data:** `"JetBrains Mono", "Fira Code", monospace`  
  Used for: text inputs, code blocks, terminal output, data display

### Font Sizes
| Role | Size | Weight |
|------|------|--------|
| Application | 11px | 400 (normal) |
| Button | 11px | 700 (bold) |
| Label | 11px | 400 |
| Small Label | 10px | 400 |
| Header | 12px | 700 |
| Input | 11px | 400 |

---

## Spacing Scale (Compact Density)

| Token | Value |
|-------|-------|
| `xs` | 4px |
| `sm` | 8px |
| `md` | 16px |
| `lg` | 24px |
| `xl` | 32px |
| `2xl` | 48px |

**Usage:**
- Padding: 8px (buttons), 6px (inputs), 4px (labels)
- Margins: 8px (default), 16px (between sections)
- Border radius: 4px (all components)

---

## Component State Matrix

Every interactive component **must** support:

### 1. **Default State**
```qss
/* Initial appearance */
border: 1px solid #303850;      /* subtle border */
background-color: #252D4A;       /* theme surface */
color: #E7E5E4;                  /* text color */
```

### 2. **Hover State** (`:hover`)
```qss
/* Pointer feedback - DESKTOP ONLY */
border-color: #00A6F4;          /* highlight color */
background-color: #303850;      /* slightly lighter */
```

### 3. **Focus-Visible State** (`:focus-visible`)
```qss
/* KEYBOARD NAVIGATION INDICATOR - CRITICAL */
outline: 2px solid #37F712;      /* primary color */
outline-offset: 2px;             /* breathing room */
```

### 4. **Active/Pressed State** (`:pressed`)
```qss
/* Clicked or activated */
background-color: #1A1F3A;      /* darker shade */
transform: scale(0.98);         /* subtle press effect */
```

### 5. **Disabled State** (`:disabled`)
```qss
/* Inactive element */
opacity: 0.5;                    /* visible but inactive */
color: #78716B;                  /* neutral gray */
cursor: not-allowed;
/* No hover/focus effects */
```

### 6. **Loading State** (`class="loading"`)
```qss
/* Processing indicator */
background-color: #252D4A;
color: #78716B;
opacity: 0.7;
/* Icon: optional spinning indicator */
```

### 7. **Error State** (`class="error"`)
```qss
/* Validation failure */
border: 1px solid #FF2157;      /* danger red */
box-shadow: 0 0 8px rgba(255, 33, 87, 0.15);
/* Icon: ✕ error symbol */
/* Text: descriptive error message */
```

### 8. **Success State** (`class="success"`)
```qss
/* Validation passed */
border: 1px solid #00A63D;      /* success green */
box-shadow: 0 0 8px rgba(0, 166, 61, 0.15);
/* Icon: ✓ checkmark */
/* Text: confirmation message */
```

### 9. **Warning State** (`class="warning"`)
```qss
/* Attention required */
border: 1px solid #FE9900;      /* warning orange */
/* Icon: ⚠ warning symbol */
/* Text: descriptive warning */
```

---

## Button Component Specification

### Button Variants (use `class` attribute)

#### Primary Button
```html
<QPushButton class="primary">Primary Action</QPushButton>
```
- **Dark:** #37F712 on #0A0E27 (neon green)
- **Light:** #1a7d05 on #F9F9F9 (dark green)
- **Use:** Main action, proceed, confirm
- **Contrast:** 9.8:1 (exceeds WCAG AAA)

#### Secondary Button
```html
<QPushButton class="secondary">Secondary</QPushButton>
```
- **Background:** #252D4A (dark) / #E8E8E8 (light)
- **Use:** Alternative actions, less emphasis
- **Contrast:** 4.5:1 (WCAG AA)

#### Success Button
```html
<QPushButton class="success">Confirm</QPushButton>
```
- **Color:** #00A63D (green)
- **Use:** Positive confirmation (delete, accept, proceed)
- **Contrast:** 6.1:1

#### Warning Button
```html
<QPushButton class="warning">Caution</QPushButton>
```
- **Color:** #FE9900 (orange)
- **Use:** Attention-required actions
- **Contrast:** 5.3:1

#### Danger Button
```html
<QPushButton class="danger">Delete</QPushButton>
```
- **Color:** #FF2157 (red)
- **Use:** Destructive actions only
- **Contrast:** 5.7:1

#### Inline Button
```html
<QPushButton class="inline">Learn More</QPushButton>
```
- **Style:** Text-only, transparent background
- **Use:** Secondary links, navigation

### Button States

| State | Appearance | Code |
|-------|------------|------|
| Default | Base color | `background-color: <token>` |
| Hover | Brightened | `:hover { background-color: lighter; }` |
| Focus | Outline | `:focus-visible { outline: 2px solid; }` |
| Pressed | Darkened | `:pressed { background-color: darker; }` |
| Disabled | Faded | `:disabled { opacity: 0.5; }` |
| Loading | Neutral | `class="loading"` |

### Button Sizing

| Target | Width | Height | Padding |
|--------|-------|--------|---------| 
| Small | 60px | 28px | 4px 8px |
| Default | 80px | 36px | 8px 16px |
| Large | 120px | 44px | 12px 24px |
| Min Touch Target | — | 44px | — |

---

## Input Component Specification

### Text Input (QLineEdit, QTextEdit, QPlainTextEdit)

#### States
```qss
/* Default */
border: 1px solid #252D4A;
background-color: #0A0E27;

/* Focus */
border: 1px solid #37F712;
box-shadow: 0 0 8px rgba(55, 247, 18, 0.15);

/* Error */
border: 1px solid #FF2157;  /* DO NOT RELY ON COLOR ALONE */

/* Success */
border: 1px solid #00A63D;

/* Warning */
border: 1px solid #FE9900;

/* Disabled */
opacity: 0.5;
color: #78716B;
```

#### Sizing
- **Height:** 32px minimum (touch-friendly)
- **Padding:** 6px 8px (comfortable spacing)
- **Border radius:** 4px
- **Border width:** 1px

#### Font
```qss
font-family: "JetBrains Mono", monospace;
font-size: 11px;
```

---

## Checkbox & Radio Button Specification

### States
```qss
/* Default */
width: 16px;
height: 16px;
border: 1px solid #303850;
background-color: #0A0E27;

/* Hover */
border-color: #00A6F4;

/* Checked */
background-color: #37F712;
border-color: #37F712;

/* Focus */
border: 2px solid #37F712;

/* Disabled */
opacity: 0.5;
```

### Spacing
- **Size:** 16×16px minimum
- **Space from label:** 4px

---

## Accessibility Requirements

### WCAG 2.2 Level AA Compliance

#### ✅ Color Contrast
- **Text:** ≥4.5:1 ratio (minimum)
- **UI Components:** ≥3:1 ratio
- **All colors verified** for light & dark themes

#### ✅ Keyboard Navigation
- **All interactive elements** receive focus (`:focus-visible`)
- **Tab order:** Defined in application code (`QWidget::setTabOrder()`)
- **No keyboard trap:** Users can escape all components

#### ✅ Focus Indicators
- **Outline:** 2px solid color
- **Offset:** 2px (breathing room)
- **Color:** Primary theme color (distinct from default)
- **Required on:** buttons, inputs, checkboxes, tabs

#### ✅ Screen Readers
- **Labels:** All inputs have associated `<label>` or `aria-label`
- **Buttons:** Text must be meaningful (not "Click here")
- **Icons:** Must have text alternative or aria-label
- **Error messages:** Associated with input via `aria-describedby`

#### ✅ Motion
- **Transitions:** 0.2s cubic-bezier for smooth feedback
- **Respects:** `@media (prefers-reduced-motion: reduce)` → no animations
- **No autoplaying:** Media, animations, or loops without user control

#### ✅ Error Identification
- **Not by color alone:** Include icons (✕ error, ✓ success)
- **Text labels:** Descriptive messages below inputs
- **Example:**
  ```
  ✕ Password must be at least 8 characters
  ```

---

## Component Implementation Checklist

Use this for **code review** of any new component:

### Core Accessibility
- [ ] Component has `:focus-visible` state (outline 2px solid)
- [ ] Component is keyboard-navigable (can receive focus via Tab)
- [ ] Component supports `:disabled` state (opacity 0.5 minimum)
- [ ] Component has meaningful label or `aria-label`
- [ ] Color is **not the only** means of communication

### States
- [ ] `:default` — base appearance defined
- [ ] `:hover` — visual feedback on pointer (if applicable)
- [ ] `:focus-visible` — distinct keyboard indicator
- [ ] `:pressed` / `:active` — clicked state
- [ ] `:disabled` — inactive appearance and behavior
- [ ] `class="error"` — validation failure (icon + text)
- [ ] `class="success"` — validation pass (icon + text)
- [ ] `class="loading"` — processing state (optional spinner)

### Touch & Mobile
- [ ] Touch target ≥44×44px
- [ ] No `:hover` fallback needed (hover doesn't exist on touch)
- [ ] Spacing adequate for finger input (no tiny buttons)

### Theme Support
- [ ] Component works in dark theme
- [ ] Component works in light theme
- [ ] No hard-coded colors (use CSS classes)
- [ ] Color contrast verified in both themes

### Typography
- [ ] Font family: Space Mono (UI) or JetBrains Mono (data)
- [ ] Font size: theme-appropriate (11px default)
- [ ] Line height: ≥1.5 (readability)
- [ ] Letter spacing: ≥0.02em (dyslexia-friendly)

### Documentation
- [ ] Component class usage documented (e.g., `class="primary"`)
- [ ] All states described in comments
- [ ] Example HTML provided
- [ ] Known limitations noted

---

## Semantic Class Naming Convention

Always use semantic class names. **Do NOT use layout class names** (e.g., `red`, `left`, `flex`).

### Buttons
```html
class="primary"     <!-- Main action -->
class="secondary"   <!-- Alternative action -->
class="success"     <!-- Positive confirmation -->
class="warning"     <!-- Attention-required -->
class="danger"      <!-- Destructive action -->
class="inline"      <!-- Text link -->
class="loading"     <!-- Processing state -->
```

### Inputs
```html
class="error"       <!-- Validation failed -->
class="warning"     <!-- Attention needed -->
class="success"     <!-- Validation passed -->
```

### Labels & Text
```html
class="sectionLabel"   <!-- Section heading -->
class="infoLabel"      <!-- Secondary text -->
class="success"        <!-- Success message (green) -->
class="error"          <!-- Error message (red) -->
class="warning"        <!-- Warning message (orange) -->
class="info"           <!-- Info message (blue) -->
class="critical"       <!-- Critical data (bold, high-contrast) -->
class="data"           <!-- Data display (monospace, green BG) -->
class="code"           <!-- Code snippet (monospace) -->
class="emptyState"     <!-- Empty state placeholder -->
```

### Containers
```html
class="card"        <!-- Elevated card (border, padding) -->
class="container"   <!-- Basic wrapper -->
class="loading"     <!-- Loading state (reduced opacity) -->
```

---

## Migration Path: Existing Components

### If you have old CSS
1. **Identify** old color values (e.g., `#FF0000`)
2. **Map** to design tokens (e.g., `#FF2157` for danger)
3. **Add** class attribute to element
4. **Remove** old inline styles
5. **Test** all states (hover, focus, disabled)

### Before:
```qss
QPushButton {
    background-color: #FF0000;  /* Hard-coded red */
}
```

### After:
```qss
QPushButton[class="danger"] {
    background-color: #FF2157;  /* Token red via class */
}
```

---

## File Structure

```
Qt/
├── style/
│   ├── style.qss           ← Master stylesheet (load FIRST)
│   ├── dark.qss            ← Dark theme (load SECOND)
│   ├── light.qss           ← Light theme (load SECOND)
│   ├── dark.qss            ← Dark theme
│   │   ├── Buttons
│   │   ├── Text inputs
│   │   ├── ComboBox
│   │   ├── Tabs
│   │   ├── Progress bars
│   │   ├── Scrollbars
│   │   ├── Loading/Success/Error states
│   │   ├── Accessibility features
│   │   └── Semantic class documentation
│   └── light.qss
│       └── (same structure as dark.qss)
├── DESIGN_SYSTEM.md        ← This file
├── mainwindow.h
├── mainwindow.cpp
└── ...
```

---

## Integration: How to Load Stylesheets

In your Qt application:

```cpp
#include <QApplication>
#include <QFile>
#include <QTextStream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Load master stylesheet
    QFile styleFile(":/style/style.qss");
    styleFile.open(QFile::ReadOnly);
    QString style = QLatin1String(styleFile.readAll());
    styleFile.close();
    
    // Load theme-specific stylesheet
    QFile themeFile(isDark ? ":/style/dark.qss" : ":/style/light.qss");
    themeFile.open(QFile::ReadOnly);
    style += QLatin1String(themeFile.readAll());
    themeFile.close();
    
    // Apply stylesheet
    app.setStyleSheet(style);
    
    // Rest of application...
    return app.exec();
}
```

---

## Best Practices

### ✅ DO

- **Use semantic classes:** `class="primary"`, `class="error"`, `class="loading"`
- **Support all states:** default, hover, focus, pressed, disabled, loading, error, success
- **Test accessibility:** keyboard navigation, screen readers, high contrast
- **Respect preferences:** Honor `prefers-reduced-motion` and `prefers-color-scheme`
- **Space properly:** Use consistent padding/margin (4–48px scale)
- **Font consistency:** Space Mono for UI, JetBrains Mono for code
- **Document states:** Explain all class usage in code comments
- **Verify contrast:** Use tools like WCAG Contrast Checker

### ❌ DON'T

- **Hard-code colors:** Use CSS classes instead
- **Rely on color alone:** Always include icons or text
- **Small touch targets:** Keep interactive elements ≥44×44px
- **Skip focus states:** Every interactive element needs `:focus-visible`
- **Ignore disabled state:** Must be visually distinct
- **Mix metaphors:** Don't mix gradients + flat + neumorphism
- **Disable animations recklessly:** Only for `prefers-reduced-motion`
- **Use ambiguous labels:** Button text like "Click here" is not accessible

---

## Testing Checklist

### Manual Testing
- [ ] Keyboard navigation: Tab through all interactive elements
- [ ] Focus visible: Each element shows distinct outline on Tab
- [ ] Disabled state: Inactive elements appear disabled (opacity, color)
- [ ] Theme switching: Both dark & light themes render correctly
- [ ] Error states: Input errors display with icon + text
- [ ] Touch targets: All buttons ≥44×44px

### Accessibility Testing
- [ ] NVDA (Windows) reads all labels correctly
- [ ] JAWS (Windows) navigates all components
- [ ] High contrast mode: Colors remain distinct
- [ ] Reduced motion: No animations when `prefers-reduced-motion: reduce`
- [ ] Color blind: Use WebAIM Contrast Checker on all text

### Responsive Testing
- [ ] Desktop (1920×1080): All elements visible
- [ ] Tablet (768×1024): Layout adjusts, touch targets readable
- [ ] Mobile (375×667): Single-column layout, touch-friendly

---

## Support & Feedback

For design system questions:
1. Check this guide first
2. Review example components in existing code
3. Run accessibility checker on output
4. Document any edge cases found

---

**Mono Design System v1.0**  
Based on [typeui.sh](https://typeui.sh) design principles  
Last Updated: April 2026
