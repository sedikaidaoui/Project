#pragma once
#include "imgui.h"

namespace font
{
	inline ImFont* icomoon = nullptr;
	inline ImFont* WeaponsIco = nullptr;
	inline ImFont* lexend_bold = nullptr;
	inline ImFont* lexend_regular = nullptr;
	inline ImFont* lexend_general_bold = nullptr;
	inline ImFont* primary_font = nullptr;
	inline ImFont* icomoon_widget = nullptr;
	inline ImFont* icomoon_widget2 = nullptr;

}

inline ImColor text_color[3] = { ImColor(255, 255, 255, 255), ImColor(200, 200, 200, 255), ImColor(150, 150, 150, 255) };
inline ImVec4 accent2 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
inline ImVec4 shadow = ImColor(0, 0, 0, 255);
inline ImColor main_color(135, 206, 250, 255);

namespace c
{
	//inline ImVec4 accent2 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	inline ImVec4 accent2 = ImVec4(0.53f, 0.81f, 0.98f, 1.00f);
	inline ImVec4 white = ImColor(255, 255, 255);

	inline ImVec4 child_bg = ImColor(13, 13, 13, 255);

	inline ImVec4 line_child = ImColor(30, 30, 30, 255);
	inline ImVec4 text_active = ImColor(255, 255, 255, 255);
	inline ImColor background_color(28, 28, 28, 150);
	inline ImVec4 accent = ImColor(135, 206, 250, 255);

	namespace text
	{ 
		inline ImVec4 text_active = ImColor(255, 255, 255);
		inline ImVec4 inregular = ImColor(138, 138, 138);
		inline ImVec4 text = ImColor(43, 51, 63);
		inline ImVec4 text_hov = ImColor(255, 255, 255);
	}

	namespace button
	{
		inline ImVec4 background = ImColor(0, 0, 0, 200);
		inline ImVec4 background_active = ImColor(206, 33, 33, 100);
		inline ImVec4 background_inactive = ImColor(25, 26, 31, 255);
		inline ImVec4 outline_background = ImColor(20, 20, 21, 255);
		inline float rounding = 4.f;
	}

	namespace background
	{

		inline ImVec4 filling = ImColor(12, 12, 12);
		inline ImVec4 stroke = ImColor(24, 26, 36);
		inline ImVec2 size = ImVec2(410, 400);
		inline ImVec2 size1 = ImVec2(700, 550);


		inline float rounding = 6;

	}

	namespace elements
	{
		constexpr ImVec4 circle_active = ImVec4(0.0f, 0.76f, 0.34f, 1.0f); // Verde neón
		constexpr ImVec4 border_hover = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
		constexpr ImColor bg_content = ImColor(24, 24, 28, 255);
		constexpr ImColor bg_widget = ImColor(40, 40, 45, 255);
		constexpr ImColor bg_hover = ImColor(50, 50, 55, 255);
		constexpr ImColor text_disabled = ImColor(150, 150, 150, 255);

		inline float rounding1 = 8;
		inline ImVec4 Circle = ImColor(99, 99, 99, 255);
		inline ImVec4 shadow = ImColor(255, 255, 255);

		inline ImVec4 mark = ImColor(255, 255, 255);

		inline ImVec4 stroke = ImColor(28, 26, 37);
		inline ImVec4 background = ImColor(15, 15, 17);
		inline ImVec4 background_widget = ImColor(21, 23, 26);

		inline ImVec4 text_active = ImColor(255, 255, 255);
		inline ImVec4 text_hov = ImColor(81, 92, 109);
		inline ImVec4 text = ImColor(43, 51, 63);

		inline float rounding = 10;
	}

	namespace child
	{

	}

	namespace tab
	{
		inline ImVec4 tab_active = ImColor(22, 22, 30);

		inline ImVec4 border = ImColor(14, 14, 15);
	}

}

//inline ImVec4 main_color = ImColor(255, 0, 0, 255);

inline ImVec4 Circle_color = ImColor(255, 255, 255, 255);





