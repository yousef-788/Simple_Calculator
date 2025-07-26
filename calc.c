#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tinyexpr.h"

#define ID_EDIT 1
#define ID_BTN_AC   1000
#define ID_BTN_DEL  1001

enum {
    ID_BTN_0 = 100, ID_BTN_1, ID_BTN_2, ID_BTN_3, ID_BTN_4,
    ID_BTN_5, ID_BTN_6, ID_BTN_7, ID_BTN_8, ID_BTN_9,
    ID_BTN_DOT, ID_BTN_PLUS, ID_BTN_MINUS, ID_BTN_MUL, ID_BTN_DIV, 
    ID_BTN_EQ, ID_BTN_CLR, ID_BTN_POW, ID_BTN_LPAREN, ID_BTN_RPAREN,
    ID_BTN_SIN, ID_BTN_COS, ID_BTN_TAN, ID_BTN_ASIN, ID_BTN_ACOS, 
    ID_BTN_ATAN, ID_BTN_SQRT, ID_BTN_LN, ID_BTN_LOG, ID_BTN_EXP,
    ID_BTN_POW10, ID_BTN_E, ID_BTN_PI, ID_BTN_MOD, ID_BTN_COMPLEX
};

typedef struct {
    const char* label;
    int id;
    int x;
    int y;
    int width;
    int height;
} ButtonDef;

HWND hEdit;
char expr[512] = "";
static HBRUSH hEditBrush = NULL;
static HBRUSH hBgBrush = NULL; 

// Matrix window handles and variables
HWND hMatrixWnd = NULL;
HWND hSizeEdit = NULL;
HWND hMatrixEdit = NULL;
HWND hResultEdit = NULL;
HWND hDetBtn = NULL;
HWND hCofBtn = NULL;
int matrixSize = 0;
double **matrix = NULL;

// Complex number window handles
HWND hComplexWnd = NULL;
HWND hReal1Edit = NULL;
HWND hImag1Edit = NULL;
HWND hReal2Edit = NULL;
HWND hImag2Edit = NULL;
HWND hComplexResultEdit = NULL;

#define DEG_TO_RAD (3.14159265358979323846 / 180.0)
#define RAD_TO_DEG (180.0 / 3.14159265358979323846)

double sin_deg(double x) {if(sin(x*DEG_TO_RAD)<0.001){return 0;}
else{return(sin(x*DEG_TO_RAD));}}
double cos_deg(double x) {if(cos(x*DEG_TO_RAD)<0.001){return 0;}
else{return(cos(x*DEG_TO_RAD));}}
double tan_deg(double x) { return tan(x * DEG_TO_RAD); }
double asin_deg(double x) { return asin(x) * RAD_TO_DEG; }
double acos_deg(double x) { return acos(x) * RAD_TO_DEG; }
double atan_deg(double x) { return atan(x) * RAD_TO_DEG; }
double ln_func(double x) { return log(x); }
double log10_func(double x) { return log10(x); }
double pow10_func(double x) { return pow(10, x); }
double exp_func(double x) { return exp(x); }

void AppendToExpr(const char* str) {
    if (strlen(expr) + strlen(str) < sizeof(expr)) {
        strcat(expr, str);
        SetWindowText(hEdit, expr);
    }
}

void ClearExpr() {
    expr[0] = '\0';
    SetWindowText(hEdit, expr);
}

void BackspaceExpr() {
    size_t len = strlen(expr);
    if (len > 0) {
        expr[len - 1] = '\0';
        SetWindowText(hEdit, expr);
    }
}

void EvaluateExpr() {
    int err;
    te_variable vars[] = {
        {"sin", sin_deg, TE_FUNCTION1}, {"cos", cos_deg, TE_FUNCTION1},
        {"tan", tan_deg, TE_FUNCTION1}, {"asin", asin_deg, TE_FUNCTION1},
        {"acos", acos_deg, TE_FUNCTION1}, {"atan", atan_deg, TE_FUNCTION1},
        {"sqrt", sqrt, TE_FUNCTION1}, {"ln", ln_func, TE_FUNCTION1},
        {"log", log10_func, TE_FUNCTION1}, {"exp", exp_func, TE_FUNCTION1},
       
    };

    te_expr* exprCompiled = te_compile(expr, vars, sizeof(vars)/sizeof(vars[0]), &err);
    if (exprCompiled) {
        double result = te_eval(exprCompiled);
        te_free(exprCompiled);
        char resultStr[50];
        sprintf(resultStr, "%g", result);
        char displayStr[600];
        sprintf(displayStr, "%s = %s", expr, resultStr);
        SetWindowText(hEdit, displayStr);
        strcpy(expr, resultStr);
    } else {
        char errMsg[100];
        sprintf(errMsg, "Error at character %d", err);
        MessageBox(NULL, errMsg, "Error", MB_OK | MB_ICONERROR);
    }
}

// Matrix functions
void FreeMatrix() {
    if (matrix) {
        for (int i = 0; i < matrixSize; i++) {
            free(matrix[i]);
        }
        free(matrix);
        matrix = NULL;
    }
    matrixSize = 0;
}

void CreateMatrix(int size) {
    FreeMatrix();
    matrixSize = size;
    matrix = (double**)malloc(size * sizeof(double*));
    for (int i = 0; i < size; i++) {
        matrix[i] = (double*)malloc(size * sizeof(double));
    }
}

void GetMatrixFromEdit() {
    if (!hMatrixEdit || !matrix || matrixSize == 0) return;

    char buffer[4096];
    GetWindowText(hMatrixEdit, buffer, sizeof(buffer));

    int row = 0, col = 0;
    char* token = strtok(buffer, " \t\n");
    while (token && row < matrixSize) {
        matrix[row][col] = atof(token);
        col++;
        if (col >= matrixSize) {
            col = 0;
            row++;
        }
        token = strtok(NULL, " \t\n");
    }
}

void GetMinor(int n, double mat[n][n], double minor[n-1][n-1], int row, int col) {
    int minorRow = 0, minorCol = 0;
    for (int i = 0; i < n; i++) {
        if (i == row) continue;
        minorCol = 0;
        for (int j = 0; j < n; j++) {
            if (j == col) continue;
            minor[minorRow][minorCol] = mat[i][j];
            minorCol++;
        }
        minorRow++;
    }
}

double CalculateDeterminant(int n, double mat[n][n]) {
    if (n == 1) return mat[0][0];
    if (n == 2) return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

    double det = 0;
    for (int col = 0; col < n; col++) {
        double minor[n-1][n-1];
        GetMinor(n, mat, minor, 0, col);
        det += (col % 2 == 0 ? 1 : -1) * mat[0][col] * CalculateDeterminant(n-1, minor);
    }
    return det;
}

void CalculateAndDisplayDeterminant() {
    if (matrixSize == 0) return;

    GetMatrixFromEdit();

    double det;
    if (matrixSize == 1) {
        det = matrix[0][0];
    } else if (matrixSize == 2) {
        det = matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    } else if (matrixSize == 3) {
        det = matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[2][1] * matrix[1][2]) -
              matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[2][0] * matrix[1][2]) +
              matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    } else {
        // For NxN matrices (N > 3)
        double tempMat[matrixSize][matrixSize];
        for (int i = 0; i < matrixSize; i++) {
            for (int j = 0; j < matrixSize; j++) {
                tempMat[i][j] = matrix[i][j];
            }
        }
        det = CalculateDeterminant(matrixSize, tempMat);
    }

    char resultStr[100];
    sprintf(resultStr, "Determinant: %g", det);
    SetWindowText(hResultEdit, resultStr);
}

void CalculateAndDisplayCofactor() {
    if (matrixSize == 0) return;

    GetMatrixFromEdit();

    double tempMat[matrixSize][matrixSize];
    for (int i = 0; i < matrixSize; i++) {
        for (int j = 0; j < matrixSize; j++) {
            tempMat[i][j] = matrix[i][j];
        }
    }

    char resultStr[4096] = "Cofactors:\r\n";
    char tempStr[50];

    for (int i = 0; i < matrixSize; i++) {
        for (int j = 0; j < matrixSize; j++) {
            if (matrixSize == 1) {
                sprintf(tempStr, "1.00 ");
            } else {
                double minor[matrixSize-1][matrixSize-1];
                GetMinor(matrixSize, tempMat, minor, i, j);
                double cofactor = ((i + j) % 2 == 0 ? 1 : -1) * CalculateDeterminant(matrixSize-1, minor);
                sprintf(tempStr, "%g ", cofactor);
            }
            strcat(resultStr, tempStr);
        }
        strcat(resultStr, "\r\n");
    }

    SetWindowText(hResultEdit, resultStr);
}

// Complex number functions
void DisplayComplexResult(double real, double imag) {
    char resultStr[256];
    sprintf(resultStr, "Result: %g + %gi", real, imag);
    SetWindowText(hComplexResultEdit, resultStr);
}

void ComplexAdd() {
    char real1Str[50], imag1Str[50], real2Str[50], imag2Str[50];
    GetWindowText(hReal1Edit, real1Str, sizeof(real1Str));
    GetWindowText(hImag1Edit, imag1Str, sizeof(imag1Str));
    GetWindowText(hReal2Edit, real2Str, sizeof(real2Str));
    GetWindowText(hImag2Edit, imag2Str, sizeof(imag2Str));

    double real1 = atof(real1Str);
    double imag1 = atof(imag1Str);
    double real2 = atof(real2Str);
    double imag2 = atof(imag2Str);

    double resultReal = real1 + real2;
    double resultImag = imag1 + imag2;

    DisplayComplexResult(resultReal, resultImag);
}

void ComplexSub() {
    char real1Str[50], imag1Str[50], real2Str[50], imag2Str[50];
    GetWindowText(hReal1Edit, real1Str, sizeof(real1Str));
    GetWindowText(hImag1Edit, imag1Str, sizeof(imag1Str));
    GetWindowText(hReal2Edit, real2Str, sizeof(real2Str));
    GetWindowText(hImag2Edit, imag2Str, sizeof(imag2Str));

    double real1 = atof(real1Str);
    double imag1 = atof(imag1Str);
    double real2 = atof(real2Str);
    double imag2 = atof(imag2Str);

    double resultReal = real1 - real2;
    double resultImag = imag1 - imag2;

    DisplayComplexResult(resultReal, resultImag);
}

void ComplexMul() {
    char real1Str[50], imag1Str[50], real2Str[50], imag2Str[50];
    GetWindowText(hReal1Edit, real1Str, sizeof(real1Str));
    GetWindowText(hImag1Edit, imag1Str, sizeof(imag1Str));
    GetWindowText(hReal2Edit, real2Str, sizeof(real2Str));
    GetWindowText(hImag2Edit, imag2Str, sizeof(imag2Str));

    double real1 = atof(real1Str);
    double imag1 = atof(imag1Str);
    double real2 = atof(real2Str);
    double imag2 = atof(imag2Str);

    double resultReal = real1 * real2 - imag1 * imag2;
    double resultImag = real1 * imag2 + imag1 * real2;

    DisplayComplexResult(resultReal, resultImag);
}

void ComplexDiv() {
    char real1Str[50], imag1Str[50], real2Str[50], imag2Str[50];
    GetWindowText(hReal1Edit, real1Str, sizeof(real1Str));
    GetWindowText(hImag1Edit, imag1Str, sizeof(imag1Str));
    GetWindowText(hReal2Edit, real2Str, sizeof(real2Str));
    GetWindowText(hImag2Edit, imag2Str, sizeof(imag2Str));

    double real1 = atof(real1Str);
    double imag1 = atof(imag1Str);
    double real2 = atof(real2Str);
    double imag2 = atof(imag2Str);

    double denom = real2 * real2 + imag2 * imag2;
    if (denom == 0) {
        MessageBox(hComplexWnd, "Cannot divide by zero", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    double resultReal = (real1 * real2 + imag1 * imag2) / denom;
    double resultImag = (imag1 * real2 - real1 * imag2) / denom;

    DisplayComplexResult(resultReal, resultImag);
}

LRESULT CALLBACK ComplexWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindow("STATIC", "First Complex Number:", WS_CHILD | WS_VISIBLE,
                        10, 10, 150, 20, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Real:", WS_CHILD | WS_VISIBLE,
                        10, 40, 50, 20, hwnd, NULL, NULL, NULL);
            hReal1Edit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    70 + 20, 40, 130, 20, hwnd, NULL, NULL, NULL); // shifted right by 20px
            CreateWindow("STATIC", "Imaginary:", WS_CHILD | WS_VISIBLE,
                        10, 70, 70, 20, hwnd, NULL, NULL, NULL);
            hImag1Edit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    80 + 20, 70, 120, 20, hwnd, NULL, NULL, NULL); // shifted right by 20px

            CreateWindow("STATIC", "Second Complex Number:", WS_CHILD | WS_VISIBLE,
                        10, 110, 170, 20, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Real:", WS_CHILD | WS_VISIBLE,
                        10, 140, 50, 20, hwnd, NULL, NULL, NULL);
            hReal2Edit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    70 + 20, 140, 130, 20, hwnd, NULL, NULL, NULL); // shifted right by 20px
            CreateWindow("STATIC", "Imaginary:", WS_CHILD | WS_VISIBLE,
                        10, 170, 70, 20, hwnd, NULL, NULL, NULL);
            hImag2Edit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    80 + 20, 170, 120, 20, hwnd, NULL, NULL, NULL); // shifted right by 20px

            CreateWindow("BUTTON", "Add", WS_CHILD | WS_VISIBLE,
                        10, 210, 70, 30, hwnd, (HMENU)1, NULL, NULL);
            CreateWindow("BUTTON", "Subtract", WS_CHILD | WS_VISIBLE,
                        90, 210, 70, 30, hwnd, (HMENU)2, NULL, NULL);
            CreateWindow("BUTTON", "Multiply", WS_CHILD | WS_VISIBLE,
                        10, 250, 70, 30, hwnd, (HMENU)3, NULL, NULL);
            CreateWindow("BUTTON", "Divide", WS_CHILD | WS_VISIBLE,
                        90, 250, 70, 30, hwnd, (HMENU)4, NULL, NULL);

            hComplexResultEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | 
                                            ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                                            10, 290, 250, 100, hwnd, NULL, NULL, NULL);

            // Set default values for testing
            SetWindowText(hReal1Edit, "");
            SetWindowText(hImag1Edit, "");
            SetWindowText(hReal2Edit, "");
            SetWindowText(hImag2Edit, "");
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case 1: // Add
                    ComplexAdd();
                    break;
                case 2: // Subtract
                    ComplexSub();
                    break;
                case 3: // Multiply
                    ComplexMul();
                    break;
                case 4: // Divide
                    ComplexDiv();
                    break;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            hComplexWnd = NULL;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK MatrixWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindow("STATIC", "Matrix Size (N):", WS_CHILD | WS_VISIBLE,
                        10, 10, 100, 20, hwnd, NULL, NULL, NULL);

            hSizeEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                    120, 10, 50, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("BUTTON", "Set Size", WS_CHILD | WS_VISIBLE,
                        180, 10, 80, 20, hwnd, (HMENU)1, NULL, NULL);

            CreateWindow("STATIC", "Matrix (space separated):", WS_CHILD | WS_VISIBLE,
                        10, 40, 200, 20, hwnd, NULL, NULL, NULL);

            hMatrixEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                                    10, 60, 250, 150, hwnd, NULL, NULL, NULL);

            hDetBtn = CreateWindow("BUTTON", "Determinant", WS_CHILD | WS_VISIBLE,
                                10, 220, 100, 30, hwnd, (HMENU)2, NULL, NULL);

            hCofBtn = CreateWindow("BUTTON", "Cofactor", WS_CHILD | WS_VISIBLE,
                                120, 220, 100, 30, hwnd, (HMENU)3, NULL, NULL);

            CreateWindow("BUTTON", "Clear", WS_CHILD | WS_VISIBLE,
                        230, 220, 60, 30, hwnd, (HMENU)4, NULL, NULL);

            hResultEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                                    10, 260, 250, 100, hwnd, NULL, NULL, NULL);
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case 1: { // Set Size button
                    char sizeStr[10];
                    GetWindowText(hSizeEdit, sizeStr, sizeof(sizeStr));
                    int size = atoi(sizeStr);
                    if (size > 0 && size <= 10) {
                        CreateMatrix(size);
                    } else {
                        MessageBox(hwnd, "Please enter a size between 1 and 10", "Invalid Size", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
                case 2: // Determinant button
                    CalculateAndDisplayDeterminant();
                    break;
                case 3: // Cofactor button
                    CalculateAndDisplayCofactor();
                    break;
                case 4: // Clear button
                    if (hMatrixEdit) SetWindowText(hMatrixEdit, "");
                    if (hResultEdit) SetWindowText(hResultEdit, "");
                    if (matrix) {
                        for (int i = 0; i < matrixSize; i++)
                            for (int j = 0; j < matrixSize; j++)
                                matrix[i][j] = 0;
                    }
                    break;
            }
            break;
        }

        case WM_CLOSE:
            FreeMatrix();
            DestroyWindow(hwnd);
            hMatrixWnd = NULL;
            break;

        case WM_DESTROY:
            FreeMatrix();
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_BTN_0: AppendToExpr("0"); break;
                case ID_BTN_1: AppendToExpr("1"); break;
                case ID_BTN_2: AppendToExpr("2"); break;
                case ID_BTN_3: AppendToExpr("3"); break;
                case ID_BTN_4: AppendToExpr("4"); break;
                case ID_BTN_5: AppendToExpr("5"); break;
                case ID_BTN_6: AppendToExpr("6"); break;
                case ID_BTN_7: AppendToExpr("7"); break;
                case ID_BTN_8: AppendToExpr("8"); break;
                case ID_BTN_9: AppendToExpr("9"); break;
                case ID_BTN_DOT: AppendToExpr("."); break;
                case ID_BTN_PLUS: AppendToExpr("+"); break;
                case ID_BTN_MINUS: AppendToExpr("-"); break;
                case ID_BTN_MUL: AppendToExpr("*"); break;
                case ID_BTN_DIV: AppendToExpr("/"); break;
                case ID_BTN_POW: AppendToExpr("^"); break;
                case ID_BTN_LPAREN: AppendToExpr("("); break;
                case ID_BTN_RPAREN: AppendToExpr(")"); break;
                case ID_BTN_SIN: AppendToExpr("sin("); break;
                case ID_BTN_COS: AppendToExpr("cos("); break;
                case ID_BTN_TAN: AppendToExpr("tan("); break;
                case ID_BTN_ASIN: AppendToExpr("asin("); break;
                case ID_BTN_ACOS: AppendToExpr("acos("); break;
                case ID_BTN_ATAN: AppendToExpr("atan("); break;
                case ID_BTN_SQRT: AppendToExpr("sqrt("); break;
                case ID_BTN_LN: AppendToExpr("ln("); break;
                case ID_BTN_LOG: AppendToExpr("log("); break;
                case ID_BTN_EXP: AppendToExpr("exp("); break;
                case ID_BTN_POW10: AppendToExpr("pow10("); break;
                case ID_BTN_E: AppendToExpr("e"); break;
                case ID_BTN_PI: AppendToExpr("pi"); break;
                case ID_BTN_CLR:
                case ID_BTN_AC: ClearExpr(); break;
                case ID_BTN_DEL: BackspaceExpr(); break;
                case ID_BTN_EQ: EvaluateExpr(); break;
                case ID_BTN_MOD: {
                    if (!hMatrixWnd) {
                        WNDCLASS wc = {0};
                        wc.lpfnWndProc = MatrixWindowProc;
                        wc.hInstance = GetModuleHandle(NULL);
                        wc.lpszClassName = "MatrixWindow";
                        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
                        RegisterClass(&wc);

                        hMatrixWnd = CreateWindow("MatrixWindow", "Matrix Operations",
                                                WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
                                                CW_USEDEFAULT, CW_USEDEFAULT, 300, 400,
                                                hwnd, NULL, GetModuleHandle(NULL), NULL);
                        ShowWindow(hMatrixWnd, SW_SHOW);
                    } else {
                        SetForegroundWindow(hMatrixWnd);
                    }
                    break;
                }
                case ID_BTN_COMPLEX: {
                    if (!hComplexWnd) {
                        WNDCLASS wc = {0};
                        wc.lpfnWndProc = ComplexWindowProc;
                        wc.hInstance = GetModuleHandle(NULL);
                        wc.lpszClassName = "ComplexWindow";
                        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
                        RegisterClass(&wc);

                        hComplexWnd = CreateWindow("ComplexWindow", "Complex Number Calculator",
                                                WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
                                                CW_USEDEFAULT, CW_USEDEFAULT, 300, 450,
                                                hwnd, NULL, GetModuleHandle(NULL), NULL);
                        ShowWindow(hComplexWnd, SW_SHOW);
                    } else {
                        SetForegroundWindow(hComplexWnd);
                    }
                    break;
                }
            }
            break;
        }

        case WM_CREATE: {
            hEditBrush = CreateSolidBrush(RGB(245, 245, 220));
            hBgBrush = CreateSolidBrush(RGB(10, 10, 60)); 

            hEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_RIGHT,
                                 13, 10, 308, 40, hwnd, (HMENU)(INT_PTR)ID_EDIT, NULL, NULL);

            ButtonDef buttons[] = {
                {"sin", ID_BTN_SIN, 13, 60, 70, 30}, 
                {"cos", ID_BTN_COS, 90, 60, 70, 30},
                {"tan", ID_BTN_TAN, 170, 60, 70, 30}, 
                {"sqrt", ID_BTN_SQRT, 250, 60, 70, 30},
                {"asin", ID_BTN_ASIN, 13, 100, 70, 30}, 
                {"acos", ID_BTN_ACOS, 90, 100, 70, 30},
                {"atan", ID_BTN_ATAN, 170, 100, 70, 30}, 
                {"^", ID_BTN_POW, 170, 180, 70, 30}, 
                {"ln", ID_BTN_LN, 13, 140, 70, 30}, 
                {"log", ID_BTN_LOG, 90, 140, 70, 30},
                {"exp", ID_BTN_EXP, 170, 140, 70, 30}, 
                {"10^x", ID_BTN_POW10, 250, 420, 70, 30},
                {"7", ID_BTN_7, 13, 300, 98, 30}, 
                {"8", ID_BTN_8, 117.66, 300, 98, 30},
                {"9", ID_BTN_9, 222.32, 300, 98, 30}, 
                {"4", ID_BTN_4, 13, 340, 98, 30},
                {"5", ID_BTN_5, 117.66, 340, 98, 30}, 
                {"6", ID_BTN_6, 222.32, 340, 98, 30},
                {"1", ID_BTN_1, 13, 380, 98, 30}, 
                {"2", ID_BTN_2, 117.66, 380, 98, 30},
                {"3", ID_BTN_3, 222.32, 380, 98, 30}, 
                {"0", ID_BTN_0, 13, 420, 70, 30},
                {".", ID_BTN_DOT, 90, 420, 70, 30}, 
                {"=", ID_BTN_EQ, 170, 420, 70, 30},
                {"AC", ID_BTN_AC, 13, 260, 150, 30}, 
                {"DEL", ID_BTN_DEL, 170, 260, 70, 30},
                {"Complex", ID_BTN_COMPLEX, 250, 260, 70, 30}, // New Complex button
                {"+", ID_BTN_PLUS, 13, 220, 70, 30}, 
                {"-", ID_BTN_MINUS, 90, 220, 70, 30},
                {"*", ID_BTN_MUL, 170, 220, 70, 30}, 
                {"/", ID_BTN_DIV, 250, 180, 70, 30},
                {"(", ID_BTN_LPAREN, 13, 180, 70, 30},
                {"Matrix", ID_BTN_MOD,  250, 220, 70, 30},
                {")", ID_BTN_RPAREN, 90, 180, 70, 30},
                {"e", ID_BTN_E, 250, 100, 70, 30},
                {"pi", ID_BTN_PI, 250, 140, 70, 30},
            };

            for (size_t i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
                CreateWindow("BUTTON", buttons[i].label, WS_CHILD | WS_VISIBLE,
                             buttons[i].x, buttons[i].y,
                             buttons[i].width, buttons[i].height,
                             hwnd, (HMENU)(INT_PTR)buttons[i].id, NULL, NULL);
            }
            break;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, RGB(245, 245, 220));
            SetTextColor(hdc, RGB(0, 0, 0));
            return (LRESULT)hEditBrush;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, hBgBrush);
            return 1;
        }
        case WM_CLOSE:  if (MessageBox(hwnd, "Are you sure you want to quit?", "Confirm Exit", 
                          MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                DestroyWindow(hwnd);
            }
            return 0; 
        case WM_DESTROY:
            DeleteObject(hEditBrush);
            DeleteObject(hBgBrush);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "SciCalc";
    wc.hbrBackground = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "SciCalc", "Scientific Calculator",
        WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT, 340, 495,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
