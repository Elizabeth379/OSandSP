#pragma once

typedef struct Color {
	int r, g, b;

	Color() { r = 0; g = 0; b = 0; }
	Color(COLORREF fromref) {
		b = (fromref >> 16) & 0xFF;
		g = (fromref >> 8) & 0xFF;
		r = (fromref & 0xFF);  }
	Color(int R, int G, int B) { r = R; g = G; b = B; }

	Color operator+(const Color& second) { return Color(r + second.r, b + second.b, g + second.g); }
	Color operator-(const Color& second) { return Color(r - second.r, b - second.b, g - second.g); }
	Color operator*(int t) { return Color(r * t, b * t, g * t); }
	Color operator/(int t) { return Color(r / t, b / t, g / t); }

	COLORREF toRGB() {
		return (b << 16) | (g << 8) | r;
	}
	Color Lerp(Color toColor, int t, int p) {
		return *this + (toColor - *this) * t / p;

	}

};

void GradientRect(HDC hDC, const RECT* lprc, Color leftColor, Color rightColor) {
	int width = lprc->left - lprc->right;

	for (int x = lprc->right; x < lprc->left; x++) {
		int curColor = leftColor.Lerp(rightColor, x - lprc->right, width).toRGB();

		for (int y = lprc->top; y < lprc->bottom; y++) {
			SetPixel(hDC, x, y, curColor);
		}
	}
}