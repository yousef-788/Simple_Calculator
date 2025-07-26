# Windows Scientific Calculator

A modern **scientific calculator** built with **C**, using the **Windows API** and the **[tinyexpr](https://github.com/codeplea/tinyexpr)** library for expression parsing and evaluation.

It supports arithmetic, trigonometric, logarithmic, exponential, and power operations — all through a clean graphical interface.

---

## 📌 Features

- ✅ Basic operations: `+`, `-`, `×`, `÷`, `^`
- 🧮 Trigonometric functions in degrees: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`
- 🧠 Scientific functions: `sqrt`, `log`, `ln`, `exp`, `10^x`
- 🎯 Constants support: `pi`, `e`
- 🖱️ Interactive GUI buttons with color-coded design:
  - 🔸 Orange for numbers
  - ⚫ Black for scientific functions
  - 🟩 Green for clear/delete
  - 🔘 Gray for operators

---

## 🖥️ How to Run (on Windows)

### 🛠️ Compile:
Using MinGW:
```bash
gcc calculator.c tinyexpr.c -o calc.exe -lgdi32
```

### ▶️ Run:
```bash
.\calc.exe
```

---

## 🧾 Dependencies

- C compiler (tested with GCC/MinGW)
- `tinyexpr.c` and `tinyexpr.h` (included)
- Windows OS (uses Win32 API)

---

## 📸 Preview

> *(Add screenshot here when available)*

---

## 📁 File Structure

```
├── calculator.c      # Main source code
├── tinyexpr.c/.h     # Expression parsing library
└── README.md         # Project documentation
```

---

## 🧑‍💻 Author

**Yousef Hamdy**  
[GitHub](https://github.com/yousef-788)  
[LinkedIn](https://www.linkedin.com/in/yousef-hamdy-ee)

---

## 📄 License

This project is licensed under the **MIT License**.
