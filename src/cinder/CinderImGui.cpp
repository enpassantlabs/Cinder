#include "cinder/CinderImGui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#include "cinder/app/App.h"
#include "cinder/Log.h"
#include "cinder/app/Window.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/app/KeyEvent.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Context.h"
#include "cinder/Clipboard.h"

#include <unordered_map>

static bool sInitialized = false;
static bool sTriggerNewFrame = false;
static std::vector<int> sAccelKeys;
static ci::signals::ConnectionList sAppConnections;
static std::unordered_map<ci::app::WindowRef, ci::signals::ConnectionList> sWindowConnections;
static bool sWindowJustCreated = false;

namespace ImGui {
	Options::Options()
		: mWindow( ci::app::getWindow() ), mAutoRender( true ), mIniPath(), mSignalPriority( 1 ), mKeyboardEnabled( true ), mGamepadEnabled( true ), mViewportsEnabled(false), mDockingEnabled(true)
	{
		//! Default Cinder styling
		mStyle.Alpha = 1.0f;                            // Global alpha applies to everything in ImGui
		mStyle.WindowPadding = ImVec2( 8, 8 );          // Padding within a window
		mStyle.WindowRounding = 7.0f;                   // Radius of window corners rounding. Set to 0.0f to have rectangular windows
		mStyle.WindowBorderSize = 1.0f;                 // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
		mStyle.WindowMinSize = ImVec2( 32, 32 );        // Minimum window size
		mStyle.WindowTitleAlign = ImVec2( 0.0f, 0.5f ); // Alignment for title bar text
		mStyle.ChildRounding = 0.0f;                    // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
		mStyle.ChildBorderSize = 1.0f;                  // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
		mStyle.PopupRounding = 0.0f;                    // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
		mStyle.PopupBorderSize = 1.0f;                  // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
		mStyle.FramePadding = ImVec2( 4, 3 );           // Padding within a framed rectangle (used by most widgets)
		mStyle.FrameRounding = 0.0f;                    // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
		mStyle.FrameBorderSize = 1.0f;                  // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
		mStyle.ItemSpacing = ImVec2( 8, 4 );            // Horizontal and vertical spacing between widgets/lines
		mStyle.ItemInnerSpacing = ImVec2( 4, 4 );       // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
		mStyle.TouchExtraPadding = ImVec2( 0, 0 );      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
		mStyle.IndentSpacing = 21.0f;                   // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
		mStyle.ColumnsMinSpacing = 6.0f;                // Minimum horizontal spacing between two columns
		mStyle.ScrollbarSize = 16.0f;                   // Width of the vertical scrollbar, Height of the horizontal scrollbar
		mStyle.ScrollbarRounding = 9.0f;                // Radius of grab corners rounding for scrollbar
		mStyle.GrabMinSize = 10.0f;                     // Minimum width/height of a grab box for slider/scrollbar
		mStyle.GrabRounding = 0.0f;                     // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
		mStyle.TabRounding = 4.0f;                      // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
		mStyle.TabBorderSize = 1.0f;                    // Thickness of border around tabs.
		mStyle.ButtonTextAlign = ImVec2( 0.5f, 0.5f );  // Alignment of button text when button is larger than text.
		mStyle.DisplayWindowPadding = ImVec2( 20, 20 ); // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
		mStyle.DisplaySafeAreaPadding = ImVec2( 3, 3 ); // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
		mStyle.MouseCursorScale = 1.0f;                 // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
		mStyle.AntiAliasedLines = true;                 // Enable anti-aliasing on lines/borders. Disable if you are really short on CPU/GPU.
		mStyle.AntiAliasedFill = true;                  // Enable anti-aliasing on filled shapes (rounded rectangles, circles, etc.)
		mStyle.CurveTessellationTol = 1.25f;            // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.


		ImVec4 grayDarkBg{ 0.07f, 0.09f, 0.08f, 0.96f };
		ImVec4 grayBg{ 0.13f, 0.15f, 0.14f, 0.96f };
		ImVec4 grayLight{ 0.29f, 0.31f, 0.30f, 0.93f };
		ImVec4 grayInactive{ 0.22f, 0.25f, 0.24f, 0.95f };
		ImVec4 orangeDimmed = grayLight;
		ImVec4 orangeActive{ 0.663f, 0.235f, 0.082f, 0.96f };

		mStyle.Colors[ImGuiCol_Text] = ImVec4( 0.93f, 0.96f, 0.95f, 0.88f );
		mStyle.Colors[ImGuiCol_TextDisabled] = ImVec4( 0.93f, 0.96f, 0.95f, 0.28f );
		mStyle.Colors[ImGuiCol_WindowBg] = grayDarkBg;
		mStyle.Colors[ImGuiCol_ChildBg] = grayDarkBg;
		mStyle.Colors[ImGuiCol_PopupBg] = ImVec4( 0.16f, 0.18f, 0.17f, 0.96f );
		mStyle.Colors[ImGuiCol_Border] = ImVec4( grayLight.x, grayLight.y, grayLight.z, 0.6f );
		mStyle.Colors[ImGuiCol_BorderShadow] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
		mStyle.Colors[ImGuiCol_FrameBg] = grayBg;
		mStyle.Colors[ImGuiCol_FrameBgHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_FrameBgActive] = orangeActive;
		mStyle.Colors[ImGuiCol_TitleBg] = grayInactive;
		mStyle.Colors[ImGuiCol_TitleBgActive] = orangeActive;
		mStyle.Colors[ImGuiCol_TitleBgCollapsed] = orangeActive;
		mStyle.Colors[ImGuiCol_MenuBarBg] = grayDarkBg;
		mStyle.Colors[ImGuiCol_ScrollbarBg] = grayBg;
		mStyle.Colors[ImGuiCol_ScrollbarGrab] = grayLight;
		mStyle.Colors[ImGuiCol_ScrollbarGrabHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_ScrollbarGrabActive] = orangeActive;
		mStyle.Colors[ImGuiCol_CheckMark] = ImVec4( 0.93f, 0.96f, 0.95f, 0.88f );
		mStyle.Colors[ImGuiCol_SliderGrab] = grayLight;
		mStyle.Colors[ImGuiCol_SliderGrabActive] = grayBg;
		mStyle.Colors[ImGuiCol_Button] = grayBg;
		mStyle.Colors[ImGuiCol_ButtonHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_ButtonActive] = orangeActive;
		mStyle.Colors[ImGuiCol_Header] = grayInactive;
		mStyle.Colors[ImGuiCol_HeaderHovered] = orangeDimmed; // orangeActive;
		mStyle.Colors[ImGuiCol_HeaderActive] = orangeActive; // orangeBright;
		mStyle.Colors[ImGuiCol_Separator] = ImVec4( 0.14f, 0.16f, 0.19f, 1.00f );
		mStyle.Colors[ImGuiCol_SeparatorHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_SeparatorActive] = orangeActive;
		mStyle.Colors[ImGuiCol_ResizeGrip] = grayBg;
		mStyle.Colors[ImGuiCol_ResizeGripHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_ResizeGripActive] = orangeActive;
		mStyle.Colors[ImGuiCol_Tab] = grayInactive;
		mStyle.Colors[ImGuiCol_TabHovered] = orangeDimmed;
		mStyle.Colors[ImGuiCol_TabActive] = orangeActive; // orangeBright;
		mStyle.Colors[ImGuiCol_TabUnfocused] = orangeDimmed;
		mStyle.Colors[ImGuiCol_TabUnfocusedActive] = orangeActive;
		//mStyle.Colors[ImGuiCol_DockingPreview] = grayInactive;
		//mStyle.Colors[ImGuiCol_DockingEmptyBg] = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );
		mStyle.Colors[ImGuiCol_PlotLines] = ImVec4( 0.93f, 0.96f, 0.95f, 0.80f );
		mStyle.Colors[ImGuiCol_PlotLinesHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.93f, 0.96f, 0.95f, 0.80f );
		mStyle.Colors[ImGuiCol_PlotHistogramHovered] = grayInactive;
		mStyle.Colors[ImGuiCol_TextSelectedBg] = grayInactive;
		mStyle.Colors[ImGuiCol_DragDropTarget] = grayInactive;
		mStyle.Colors[ImGuiCol_NavHighlight] = grayInactive;
		mStyle.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
		mStyle.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
		mStyle.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.20f, 0.22f, 0.27f, 0.73f );
	}

	Options& Options::window( const ci::app::WindowRef& window, int signalPriority )
	{
		mWindow = window;
		mSignalPriority = signalPriority;
		return *this;
	}

	Options& Options::autoRender( bool autoRender )
	{
		mAutoRender = autoRender;
		return *this;
	}

	Options& Options::iniPath( const ci::fs::path& path )
	{
		mIniPath = path;
		return *this;
	}

	Options& Options::enableKeyboard( bool enable )
	{
		mKeyboardEnabled = enable;
		return *this;
	}

	Options& Options::enableGamepad( bool enable )
	{
		mGamepadEnabled = enable;
		return *this;
	}

	Options& Options::enableViewports(bool enable)
	{
		mViewportsEnabled = enable;
		return *this;
	}

	Options& Options::enableDocking(bool enable)
	{
		mDockingEnabled = enable;
		return *this;
	}

	Options& Options::signalPriority( int signalPriority )
	{
		mSignalPriority = signalPriority;
		return *this;
	}

	Options& Options::style( const ImGuiStyle& style )
	{
		mStyle = style;
		return *this;
	}

	ScopedId::ScopedId( int int_id )
	{
		ImGui::PushID( int_id );
	}

	ScopedId::ScopedId( const char* label )
	{
		ImGui::PushID( label );
	}

	ScopedId::ScopedId( const void *ptrId )
	{
		ImGui::PushID( ptrId );
	}

	ScopedId::~ScopedId()
	{
		ImGui::PopID();
	}

	ScopedWindow::ScopedWindow(const char* label, ImGuiWindowFlags flags, bool *open) 
		: mOpened(ImGui::Begin(label, open, flags))
	{
	}

	ScopedWindow::~ScopedWindow()
	{
		ImGui::End();
	}

	ScopedMenuBar::ScopedMenuBar(bool isMainMenu) 
		: mIsMainMenu(isMainMenu)
	{
		mOpened = mIsMainMenu ? ImGui::BeginMainMenuBar() : ImGui::BeginMenuBar();
	}

	ScopedMenuBar::~ScopedMenuBar()
	{
		if (mOpened && mIsMainMenu) ImGui::EndMainMenuBar();
		if( mOpened && !mIsMainMenu ) ImGui::EndMenuBar();
	}

	ScopedMenu::ScopedMenu(const char* label, bool enabled) 
		: mOpened{ ImGui::BeginMenu(label, enabled) } 
	{
	}

	ScopedMenu::~ScopedMenu()
	{
		if (mOpened) ImGui::EndMenu();
	}

	ScopedGroup::ScopedGroup()
	{
		ImGui::BeginGroup();
	}
	
	ScopedGroup::~ScopedGroup()
	{
		ImGui::EndGroup();
	}
	
	ScopedTreeNode::ScopedTreeNode(const std::string& name)
		: mOpened(ImGui::TreeNode(name.c_str()))
	{
	}

	ScopedTreeNode::~ScopedTreeNode()
	{
		if (mOpened) ImGui::TreePop();
	}

	ScopedCollapsingHeader::ScopedCollapsingHeader(const std::string& name, ImGuiTreeNodeFlags flags)
		: mOpened(ImGui::CollapsingHeader(name.c_str(), flags))
	{
	}

	ScopedCollapsingHeader::~ScopedCollapsingHeader()
	{
	}
	
	ScopedColumns::ScopedColumns( int count, const char* id, bool border )
	{
		ImGui::Columns( count, id, border );
	}
	
	ScopedColumns::~ScopedColumns()
	{
		ImGui::NextColumn();
	}
	
	ScopedStyleVar::ScopedStyleVar(ImGuiStyleVar styleVar, float value) {
		ImGui::PushStyleVar(styleVar, value);
	}

	ScopedStyleVar::ScopedStyleVar(ImGuiStyleVar styleVar, const ImVec2 &value) {
		ImGui::PushStyleVar(styleVar, value);
	}

	ScopedStyleVar::~ScopedStyleVar() {
		ImGui::PopStyleVar();
	}

	ScopedItemWidth::ScopedItemWidth(float width) {
		ImGui::PushItemWidth(width);
	}

	ScopedItemWidth::~ScopedItemWidth() {
		ImGui::PopItemWidth();
	}

	ScopedIndent::ScopedIndent(float indent)
		: mIndent(indent) {
		ImGui::Indent(indent);
	}

	ScopedIndent::~ScopedIndent() {
		ImGui::Indent(-mIndent);
	}

	bool DragFloat2( const char* label, glm::vec2* v2, float v_speed, float v_min, float v_max, const char* format, float power ) {
		return DragFloat2( label, glm::value_ptr( *v2 ), v_speed, v_min, v_max, format, power );
	}

	bool DragFloat3( const char* label, glm::vec3* v3, float v_speed, float v_min, float v_max, const char* format, float power ) {
		return DragFloat3( label, glm::value_ptr( *v3 ), v_speed, v_min, v_max, format, power );
	}

	bool DragFloat4( const char* label, glm::vec4* v4, float v_speed, float v_min, float v_max, const char* format, float power ) {
		return DragFloat4( label, glm::value_ptr( *v4 ), v_speed, v_min, v_max, format, power );
	}

	bool DragInt2( const char* label, glm::ivec2* v2, float v_speed, int v_min, int v_max, const char* format ) {
		return DragInt2( label, glm::value_ptr( *v2 ), v_speed, v_min, v_max, format );
	}

	bool DragInt3( const char* label, glm::ivec3* v3, float v_speed, int v_min, int v_max, const char* format ) {
		return DragInt3( label, glm::value_ptr( *v3 ), v_speed, v_min, v_max, format );
	}

	bool DragInt4( const char* label, glm::ivec4* v4, float v_speed, int v_min, int v_max, const char* format ) {
		return DragInt4( label, glm::value_ptr( *v4 ), v_speed, v_min, v_max, format );
	}

	bool SliderFloat2( const char* label, glm::vec2* v2, float v_min, float v_max, const char* format, float power ) {
		return SliderFloat2( label, glm::value_ptr( *v2 ), v_min, v_max, format, power );
	}

	bool SliderFloat3( const char* label, glm::vec3* v3, float v_min, float v_max, const char* format, float power ) {
		return SliderFloat3( label, glm::value_ptr( *v3 ), v_min, v_max, format, power );
	}

	bool SliderFloat4( const char* label, glm::vec4* v4, float v_min, float v_max, const char* format, float power ) {
		return SliderFloat4( label, glm::value_ptr( *v4 ), v_min, v_max, format, power );
	}

	bool SliderInt2( const char* label, glm::ivec2* v2, int v_min, int v_max, const char* format ) {
		return SliderInt2( label, glm::value_ptr( *v2 ), v_min, v_max, format );
	}

	bool SliderInt3( const char* label, glm::ivec3* v3, int v_min, int v_max, const char* format ) {
		return SliderInt3( label, glm::value_ptr( *v3 ), v_min, v_max, format );
	}

	bool SliderInt4( const char* label, glm::ivec4* v4, int v_min, int v_max, const char* format ) {
		return SliderInt4( label, glm::value_ptr( *v4 ), v_min, v_max, format );
	}

	bool InputInt2( const char* label, glm::ivec2* v2, ImGuiInputTextFlags flags ) {
		return InputInt2( label, glm::value_ptr( *v2 ), flags );
	}

	bool InputInt3( const char* label, glm::ivec3* v3, ImGuiInputTextFlags flags ) {
		return InputInt3( label, glm::value_ptr( *v3 ), flags );
	}

	bool InputInt4( const char* label, glm::ivec4* v4, ImGuiInputTextFlags flags ) {
		return InputInt4( label, glm::value_ptr( *v4 ), flags );
	}

	bool ColorEdit3( const char* label, ci::Colorf* color, ImGuiColorEditFlags flags ) {
		return ColorEdit3( label, color->ptr(), flags );
	}

	bool ColorEdit4( const char* label, ci::ColorAf* color, ImGuiColorEditFlags flags ) {
		return ColorEdit4( label, color->ptr(), flags );
	}

	bool ColorPicker3( const char* label, ci::Colorf* color, ImGuiColorEditFlags flags ) {
		return ColorPicker3( label, color->ptr(), flags );
	}

	bool ColorPicker4( const char* label, ci::ColorAf* color, ImGuiColorEditFlags flags ) {
		return ColorPicker4( label, color->ptr(), flags );
	}

	bool Combo( const char* label, int* currIndex, const std::vector<std::string>& values, ImGuiComboFlags flags )
	{
		if( values.empty() ) return false;

		bool changed = false;
		int itemsCount = (int)values.size();
		const char* previewItem = NULL;
		if( *currIndex >= 0 && *currIndex < itemsCount ) {
			previewItem = values.at( *currIndex ).c_str();
		}

		if( ImGui::BeginCombo( label, previewItem, flags ) ) {
			for( int i = 0; i < itemsCount; ++i ) {
				ImGui::PushID( (void*)(intptr_t)i );
				bool selected = ( *currIndex == i );
				if( ImGui::Selectable( values.at( i ).c_str(), selected ) ) {
					*currIndex = i;
					changed = true;
				}
				if( selected ) ImGui::SetItemDefaultFocus();
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	bool ListBox( const char* label, int* currIndex, const std::vector<std::string>& values, int height_in_items )
	{
		if( values.empty() ) return false;
		
		bool changed = false;
		if( ImGui::ListBoxHeader( label, (int)values.size(), height_in_items ) ) {
			for( int i = 0; i < (int)values.size(); ++i ) {
				ImGui::PushID( (void*)(intptr_t)i );
				bool selected = ( *currIndex == i );
				if( ImGui::Selectable( values.at( i ).c_str(), selected ) ) {
					*currIndex = i;
					changed = true;
				}
				if( selected ) ImGui::SetItemDefaultFocus();
				ImGui::PopID();
			}
			ImGui::ListBoxFooter();
		}
		return changed;
	}

	void Image( const ci::gl::Texture2dRef& texture, const ci::vec2& size, const ci::vec2& uv0, const ci::vec2& uv1, const ci::vec4& tint_col, const ci::vec4& border_col )
	{
		Image( (void*)(intptr_t)texture->getId(), size, uv0, uv1, tint_col, border_col );
	}

	void PopupModal(const char* label, const char* message, std::function<void()> confirmFn, std::function<void()> cancelFn, const char* confirmLabel, const char* cancelLabel) {
		ImGui::SetNextWindowSize(ImVec2(500, 0));
		if (ImGui::BeginPopupModal(label, 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400);
			ImGui::Text(message);
			ImGui::NewLine();

			if (ImGui::Button(confirmLabel, ImVec2((confirmFn || cancelFn) ? ImGui::GetWindowContentRegionWidth() / 2.0 - 5 : ImGui::GetWindowContentRegionWidth(), 0))) {
				if (confirmFn) {
					confirmFn();
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			// if no callbacks are provided we only need 1 button
			if(confirmFn || cancelFn) {
				ImGui::SameLine();
				if (ImGui::Button(cancelLabel, ImVec2(ImGui::GetWindowContentRegionWidth() / 2.0 - 5, 0))) {
					if (cancelFn) {
						cancelFn();
					}
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}


	/*
	* Logger
	*/
	// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
	struct ExampleAppLog
	{
		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		bool                AutoScroll;  // Keep scrolling if already at the bottom.
		bool				NewLog;		// did we get a new log and not autoscroll yet?

		ExampleAppLog()
		{
			AutoScroll = true;
			NewLog = false;
			Clear();
		}

		void    Clear()
		{
			Buf.clear();
			LineOffsets.clear();
			LineOffsets.push_back(0);
		}

		void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
		{
			int old_size = Buf.size();
			va_list args;
			va_start(args, fmt);
			Buf.appendfv(fmt, args);
			va_end(args);
			for (int new_size = Buf.size(); old_size < new_size; old_size++)
				if (Buf[old_size] == '\n')
					LineOffsets.push_back(old_size + 1);

			NewLog = true;
		}

		void    Draw(const char* title, bool* p_open = NULL)
		{
			if (!ImGui::Begin(title, p_open))
			{
				ImGui::End();
				return;
			}

			// Auto scroll
			ImGui::Checkbox("Auto-scroll", &AutoScroll);			
			ImGui::SameLine();

			// Main window
			bool clear = ImGui::Button("Clear");
			ImGui::SameLine();
			bool copy = ImGui::Button("Copy");
			ImGui::SameLine();
			Filter.Draw("Filter", -100.0f);

			ImGui::Separator();
			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			if (clear)
				Clear();
			if (copy)
				ImGui::LogToClipboard();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			const char* buf = Buf.begin();
			const char* buf_end = Buf.end();
			if (Filter.IsActive())
			{
				// In this example we don't use the clipper when Filter is enabled.
				// This is because we don't have a random access on the result on our filter.
				// A real application processing logs with ten of thousands of entries may want to store the result of
				// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
				for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					if (Filter.PassFilter(line_start, line_end))
						ImGui::TextUnformatted(line_start, line_end);
				}
			}
			else
			{
				// The simplest and easy way to display the entire buffer:
				//   ImGui::TextUnformatted(buf_begin, buf_end);
				// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
				// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
				// within the visible area.
				// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
				// on your side is recommended. Using ImGuiListClipper requires
				// - A) random access into your data
				// - B) items all being the  same height,
				// both of which we can handle since we an array pointing to the beginning of each line of text.
				// When using the filter (in the block of code above) we don't have random access into the data to display
				// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
				// it possible (and would be recommended if you want to search through tens of thousands of entries).
				ImGuiListClipper clipper;
				clipper.Begin(LineOffsets.Size);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = buf + LineOffsets[line_no];
						const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
						ImGui::TextUnformatted(line_start, line_end);
					}
				}
				clipper.End();
			}
			ImGui::PopStyleVar();

			if (copy)
				ImGui::LogFinish();

			if (AutoScroll && NewLog) {
				ImGui::SetScrollHereY(1.0f);
				NewLog = false;
			}

			ImGui::EndChild();
			ImGui::End();
		}
	};

	const std::string getCurrentDateTimeString()
	{
		time_t timeSinceEpoch = time( NULL );
		struct tm *now = localtime( &timeSinceEpoch );

		char result[100];
		strftime( result, sizeof( result ), "%Y-%m-%d.%X", now );

		return result;
	}

	static ExampleAppLog log;

	Logger::Logger() : mLogLevel(ci::log::LEVEL_DEBUG)
	{
	}

	void Logger::write(const ci::log::Metadata& meta, const std::string& text)
	{
		
		if (meta.mLevel >= mLogLevel) {
			if(isTimestampEnabled()) {
				log.AddLog("%s   %s : %s\n", getCurrentDateTimeString().c_str(), meta.toString().c_str(), text.c_str());
			}
			else {
				log.AddLog("%s : %s\n", meta.toString().c_str(), text.c_str());
			}
		}
	}

	void Logger::draw(bool *open)
	{
		log.Draw("Log", open);
	}
}

struct ImGuiViewportDataCinder
{
	ci::app::WindowRef window;
	ci::app::Window::Format format;

	ImGuiViewportDataCinder() { window = nullptr; }
	~ImGuiViewportDataCinder() { IM_ASSERT(window == nullptr); }
};


static void ImGui_ImplCinder_WindowCloseCallback(ci::app::WindowRef window) {
	if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window.get())) {
		viewport->PlatformRequestClose = true;
	}
}

static void ImGui_ImplCinder_WindowPosCallback(ci::app::WindowRef window, int x, int y)
{
	if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window.get()))
	{
		viewport->PlatformRequestMove = true;
	}
}

static void ImGui_ImplCinder_WindowSizeCallback(ci::app::WindowRef window, int w, int h)
{
	if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window.get()))
	{
		viewport->PlatformRequestResize = true;
	}
}

static void ImGui_ImplCinder_MouseDown(ci::app::MouseEvent& event);
static void ImGui_ImplCinder_MouseUp(ci::app::MouseEvent& event);
static void ImGui_ImplCinder_MouseMove(ci::app::MouseEvent& event);
static void ImGui_ImplCinder_MouseDrag(ci::app::MouseEvent& event);
static void ImGui_ImplCinder_MouseWheel(ci::app::MouseEvent& event);
static void ImGui_ImplCinder_KeyDown(ci::app::KeyEvent& event);
static void ImGui_ImplCinder_KeyUp(ci::app::KeyEvent& event);

static void ImGui_ImplCinder_CreateWindow(ImGuiViewport* viewport)
{
	// we use this to ignore the mouseup event generated by the main window when it loses focus
	// we only want to ignore this if the mouse keys are down
	ImGuiIO& io = ImGui::GetIO();
	sWindowJustCreated = io.MouseDown[0];

	ImGuiViewportDataCinder* data = IM_NEW(ImGuiViewportDataCinder)();
	viewport->PlatformUserData = data;

	// create format
	data->format = ci::app::Window::Format().pos(viewport->Pos).size(viewport->Size).borderless();

	// create window
	auto window = ci::app::App::get()->createWindow(data->format);
	sWindowConnections[window] += window->getSignalClose().connect([window]() {
		ImGui_ImplCinder_WindowCloseCallback(window);
		});

	sWindowConnections[window] += window->getSignalMove().connect([window]() {
		ImGui_ImplCinder_WindowPosCallback(window, window->getPos().x, window->getPos().y);
		});

	sWindowConnections[window] += window->getSignalResize().connect([window]() {
		ImGui_ImplCinder_WindowSizeCallback(window, window->getSize().x, window->getSize().y);
		});

	sWindowConnections[window] += window->getSignalMouseDown().connect(ImGui_ImplCinder_MouseDown);
	sWindowConnections[window] += window->getSignalMouseUp().connect(ImGui_ImplCinder_MouseUp);
	sWindowConnections[window] += window->getSignalMouseMove().connect(ImGui_ImplCinder_MouseMove);
	sWindowConnections[window] += window->getSignalMouseDrag().connect(ImGui_ImplCinder_MouseDrag);
	sWindowConnections[window] += window->getSignalMouseWheel().connect(ImGui_ImplCinder_MouseWheel);
	sWindowConnections[window] += window->getSignalKeyDown().connect(ImGui_ImplCinder_KeyDown);
	sWindowConnections[window] += window->getSignalKeyUp().connect(ImGui_ImplCinder_KeyUp);

	data->window = window;
	viewport->PlatformRequestResize = false;
	viewport->PlatformHandle = window.get();
	viewport->PlatformHandleRaw = window->getNative();

	window->getRenderer()->makeCurrentContext();
}

static void ImGui_ImplCinder_DestroyWindow(ImGuiViewport* viewport)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData)
	{
		if (data->window) {
			sWindowConnections.erase(data->window);
			if (!ci::app::App::get()->getQuitRequested()) {
				data->window->close();
			}
			data->window = nullptr;
		}
		IM_DELETE(data);

		viewport->PlatformUserData = nullptr;
		viewport->PlatformHandle = nullptr;
		viewport->PlatformHandleRaw = nullptr;
	}
}

static void ImGui_ImplCinder_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData)
	{
		if (data->window) {
			data->window->setPos(pos);
		}
	}
}

static ImVec2 ImGui_ImplCinder_GetWindowPos(ImGuiViewport* viewport)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData)
	{
		IM_ASSERT(data->window != 0);
		return data->window->getPos();
	}

	return ImVec2();
}

static void ImGui_ImplCinder_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData)
	{
		if (data->window) {
			data->window->setSize(size);
		}
	}
}

static ImVec2 ImGui_ImplCinder_GetWindowSize(ImGuiViewport* viewport)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData)
	{
		IM_ASSERT(data->window != 0);
		return data->window->getSize();
	}

	return ImVec2();
}

static float ImGui_ImplCinder_GetWindowDpiScale(ImGuiViewport* viewport)
{
	ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData;
	IM_ASSERT(data->window != 0);
	auto display = data->window->getDisplay();
	return display->getContentScale();
}

static void ImGui_ImplCinder_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData) {
		if (data->window) {
			data->window->setTitle(title);
		}
	}
}

static void ImGui_ImplCinder_UpdateWindow(ImGuiViewport* viewport)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData) {
		IM_ASSERT(data->window != 0);

		data->format = ci::app::Window::Format().pos(viewport->Pos).size(viewport->Size);
		if (data->window->getPos() != data->format.getPos()) {
			data->window->setPos(data->format.getPos());
			viewport->PlatformRequestMove = true;
		}
		if (data->window->getSize() != data->format.getSize()) {
			data->window->setSize(data->format.getSize());
			viewport->PlatformRequestResize = true;
		}
	}
}

static void ImGui_ImplCinder_ShowWindow(ImGuiViewport* viewport)
{
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData) {
		if (data->window) {
			data->window->show();
		}
	}
}

static void ImGui_ImplCinder_RenderWindow(ImGuiViewport* viewport, void*) {
	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData) {
		if (data->window) {
			data->window->getRenderer()->makeCurrentContext();
		}
	}
}

static void ImGui_ImplCinder_SwapBuffers(ImGuiViewport* viewport, void*) {
	return;

	if (ImGuiViewportDataCinder* data = (ImGuiViewportDataCinder*)viewport->PlatformUserData) {
		if (data->window) {
			data->window->getRenderer()->makeCurrentContext();
			data->window->getRenderer()->swapBuffers();
		}
	}
}

static void ImGui_ImplCinder_UpdateMonitors()
{
	ImGui::GetPlatformIO().Monitors.resize(0);
	auto displays = ci::Display::getDisplays();
	for (auto d : displays) {
		ImGuiPlatformMonitor imgui_monitor;
		auto b = d->getBounds();
		imgui_monitor.MainPos = ImVec2((float)b.x1, (float)b.y1);
		imgui_monitor.MainSize = ImVec2((float)(b.getWidth()), (float)(b.getHeight()));
		imgui_monitor.WorkPos = imgui_monitor.MainPos;
		imgui_monitor.WorkSize = imgui_monitor.MainSize;
		imgui_monitor.DpiScale = d->getContentScale();
		ImGuiPlatformIO& io = ImGui::GetPlatformIO();
		io.Monitors.push_back(imgui_monitor);
	}
}

static void ImGui_ImplCinder_InitPlatformInterface() {
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

	platform_io.Platform_CreateWindow = ImGui_ImplCinder_CreateWindow;
	platform_io.Platform_DestroyWindow = ImGui_ImplCinder_DestroyWindow;
	platform_io.Platform_ShowWindow = ImGui_ImplCinder_ShowWindow;
	platform_io.Platform_SetWindowPos = ImGui_ImplCinder_SetWindowPos;
	platform_io.Platform_GetWindowPos = ImGui_ImplCinder_GetWindowPos;
	platform_io.Platform_SetWindowSize = ImGui_ImplCinder_SetWindowSize;
	platform_io.Platform_GetWindowSize = ImGui_ImplCinder_GetWindowSize;
	platform_io.Platform_GetWindowDpiScale = ImGui_ImplCinder_GetWindowDpiScale;
	platform_io.Platform_UpdateWindow = ImGui_ImplCinder_UpdateWindow;
	platform_io.Platform_SetWindowTitle = ImGui_ImplCinder_SetWindowTitle;
	platform_io.Platform_RenderWindow = ImGui_ImplCinder_RenderWindow;
	platform_io.Platform_SwapBuffers = ImGui_ImplCinder_SwapBuffers;
}

static void ImGui_ImplCinder_MouseDown( ci::app::MouseEvent& event )
{
	ImGuiIO& io = ImGui::GetIO();
	glm::vec2 pos = ci::app::toPixels( event.getPos() );
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		pos += event.getWindow()->getPos();
	}
	io.MousePos = pos;
	io.MouseDown[0] = event.isLeftDown();
	io.MouseDown[1] = event.isRightDown();
	io.MouseDown[2] = event.isMiddleDown();
	event.setHandled( io.WantCaptureMouse );
}
static void ImGui_ImplCinder_MouseUp( ci::app::MouseEvent& event )
{
	// we ignore mouseup events generated by the main window losing focus if a new window is spawned
	if (!sWindowJustCreated) {
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[0] = false;
		io.MouseDown[1] = false;
		io.MouseDown[2] = false;
	}
	else {
		sWindowJustCreated = false;
	}
}
static void ImGui_ImplCinder_MouseWheel( ci::app::MouseEvent& event )
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheel += event.getWheelIncrement();
	event.setHandled( io.WantCaptureMouse );
}

static void ImGui_ImplCinder_MouseMove( ci::app::MouseEvent& event )
{
	ImGuiIO& io = ImGui::GetIO();
	glm::vec2 pos = ci::app::toPixels(event.getPos());
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		pos += event.getWindow()->getPos();
	}
	io.MousePos = pos;
	event.setHandled( io.WantCaptureMouse );
}

//! sets the right mouseDrag IO values in imgui
static void ImGui_ImplCinder_MouseDrag( ci::app::MouseEvent& event )
{
	ImGuiIO& io = ImGui::GetIO();
	glm::vec2 pos = ci::app::toPixels(event.getPos());
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		pos += event.getWindow()->getPos();
	}
	io.MousePos = pos;
	event.setHandled( io.WantCaptureMouse );
}

static void ImGui_ImplCinder_KeyDown( ci::app::KeyEvent& event )
{
	ImGuiIO& io = ImGui::GetIO();

#if defined CINDER_LINUX
	auto character = event.getChar();
#else
	uint32_t character = event.getCharUtf32();
#endif

	io.KeysDown[event.getCode()] = true;

	if( !event.isAccelDown() && character > 0 && character <= 255 ) {
		io.AddInputCharacter( (char)character );
	}
	else if( event.getCode() != ci::app::KeyEvent::KEY_LMETA
		&& event.getCode() != ci::app::KeyEvent::KEY_RMETA
		&& event.isAccelDown()
		&& find( sAccelKeys.begin(), sAccelKeys.end(), event.getCode() ) == sAccelKeys.end() ) {
		sAccelKeys.push_back( event.getCode() );
	}

	io.KeyCtrl = event.isControlDown();
	io.KeyShift = event.isShiftDown();
	io.KeyAlt = event.isAltDown();
	io.KeySuper = event.isMetaDown();

	event.setHandled( io.WantTextInput );
}

static void ImGui_ImplCinder_KeyUp( ci::app::KeyEvent& event )
{
	ImGuiIO& io = ImGui::GetIO();

	io.KeysDown[event.getCode()] = false;

	for( auto key : sAccelKeys ) {
		io.KeysDown[key] = false;
	}
	sAccelKeys.clear();

	io.KeyCtrl = event.isControlDown();
	io.KeyShift = event.isShiftDown();
	io.KeyAlt = event.isAltDown();
	io.KeySuper = event.isMetaDown();

	event.setHandled( io.WantTextInput );
}

static void ImGui_ImplCinder_NewFrameGuard( const ci::app::WindowRef& window );

static void ImGui_ImplCinder_Resize( const ci::app::WindowRef& window )
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ci::vec2( window->toPixels( window->getSize() ) );

	ImGui_ImplCinder_NewFrameGuard( window );
}

static void ImGui_ImplCinder_Move() {
	ImGui_ImplCinder_NewFrameGuard(ci::app::getWindow());
}

static void ImGui_ImplCinder_NewFrameGuard( const ci::app::WindowRef& window ) {
	if( ! sTriggerNewFrame )
		return;

	ImGui_ImplOpenGL3_NewFrame();
	
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT( io.Fonts->IsBuilt() ); // Font atlas needs to be built, call renderer _NewFrame() function e.g. ImGui_ImplOpenGL3_NewFrame() 

	// Setup display size
	//io.DisplaySize = window->toPixels( window->getSize() );
	//io.DisplaySize = ci::app::toPixels( window->getSize() );

	// Setup time step
	static double g_Time = 0.0f;
	double current_time = ci::app::getElapsedSeconds();
	io.DeltaTime = g_Time > 0.0 ? (float)( current_time - g_Time ) : (float)( 1.0f / 60.0f );
	g_Time = current_time;

	ImGui::NewFrame();

	sTriggerNewFrame = false;
}

static void ImGui_ImplCinder_PostDraw()
{
	ImGui::Render();
	
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		auto ctx = ci::gl::context();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		ctx->makeCurrent();
	}

	sTriggerNewFrame = true;
}

static bool ImGui_ImplCinder_Init( const ci::app::WindowRef& window, const ImGui::Options& options )
{
	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "imgui_impl_cinder";
	if(options.isViewportsEnabled()) io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = ci::app::KeyEvent::KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = ci::app::KeyEvent::KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = ci::app::KeyEvent::KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = ci::app::KeyEvent::KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = ci::app::KeyEvent::KEY_DOWN;
	io.KeyMap[ImGuiKey_Home] = ci::app::KeyEvent::KEY_HOME;
	io.KeyMap[ImGuiKey_End] = ci::app::KeyEvent::KEY_END;
	io.KeyMap[ImGuiKey_Delete] = ci::app::KeyEvent::KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = ci::app::KeyEvent::KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = ci::app::KeyEvent::KEY_RETURN;
	io.KeyMap[ImGuiKey_Escape] = ci::app::KeyEvent::KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = ci::app::KeyEvent::KEY_a;
	io.KeyMap[ImGuiKey_C] = ci::app::KeyEvent::KEY_c;
	io.KeyMap[ImGuiKey_V] = ci::app::KeyEvent::KEY_v;
	io.KeyMap[ImGuiKey_X] = ci::app::KeyEvent::KEY_x;
	io.KeyMap[ImGuiKey_Y] = ci::app::KeyEvent::KEY_y;
	io.KeyMap[ImGuiKey_Z] = ci::app::KeyEvent::KEY_z;
	io.KeyMap[ImGuiKey_Insert] = ci::app::KeyEvent::KEY_INSERT;
	io.KeyMap[ImGuiKey_Space] = ci::app::KeyEvent::KEY_SPACE;

	ImGuiStyle& imGuiStyle = ImGui::GetStyle();
	imGuiStyle = options.getStyle();

#ifndef CINDER_LINUX
	// clipboard callbacks
	io.SetClipboardTextFn = []( void* user_data, const char* text ) {
		ci::Clipboard::setString( std::string( text ) );
	};
	io.GetClipboardTextFn = []( void* user_data ) {
		std::string str = ci::Clipboard::getString();
		static std::vector<char> strCopy;
		strCopy = std::vector<char>( str.begin(), str.end() );
		strCopy.push_back( '\0' );
		return (const char*)&strCopy[0];
	};
#endif
	int signalPriority = options.getSignalPriority();
	sWindowConnections[window] += window->getSignalMouseDown().connect( signalPriority, ImGui_ImplCinder_MouseDown );
	sWindowConnections[window] += window->getSignalMouseUp().connect( signalPriority, ImGui_ImplCinder_MouseUp );
	sWindowConnections[window] += window->getSignalMouseMove().connect( signalPriority, ImGui_ImplCinder_MouseMove );
	sWindowConnections[window] += window->getSignalMouseDrag().connect( signalPriority, ImGui_ImplCinder_MouseDrag );
	sWindowConnections[window] += window->getSignalMouseWheel().connect( signalPriority, ImGui_ImplCinder_MouseWheel );
	sWindowConnections[window] += window->getSignalKeyDown().connect( signalPriority, ImGui_ImplCinder_KeyDown );
	sWindowConnections[window] += window->getSignalKeyUp().connect( signalPriority, ImGui_ImplCinder_KeyUp );
	sWindowConnections[window] += window->getSignalResize().connect( signalPriority, std::bind( ImGui_ImplCinder_Resize, window ) );
	sWindowConnections[window] += window->getSignalMove().connect(signalPriority, ImGui_ImplCinder_Move);
	if( options.isAutoRenderEnabled() ) {
		sWindowConnections[window] += ci::app::App::get()->getSignalUpdate().connect( std::bind( ImGui_ImplCinder_NewFrameGuard, window ) );
		sWindowConnections[window] += window->getSignalPostDraw().connect( ImGui_ImplCinder_PostDraw );
	}

	sWindowConnections[window] += window->getSignalClose().connect( [=] {
		sWindowConnections.erase( window );
		sTriggerNewFrame = false;
		ci::app::App::get()->quit();
		ci::app::App::get()->setQuitRequested();
	} );

	if(options.isViewportsEnabled()) {
		// Register main window handle (which is owned by the main application, not by us)
		// This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
		ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGuiViewportDataCinder* data = IM_NEW(ImGuiViewportDataCinder)();
		data->window = window;
		main_viewport->PlatformUserData = data;
		main_viewport->PlatformHandle = (void*)data->window.get();
		main_viewport->PlatformHandleRaw = data->window->getNative();

		ImGui_ImplCinder_UpdateMonitors();
		ImGui_ImplCinder_InitPlatformInterface();
	}

	return true;
}

static void ImGui_ImplCinder_Shutdown()
{
	sWindowConnections.clear();
}

bool ImGui::Initialize( const ImGui::Options& options )
{
	if( sInitialized )
		return false;

	IMGUI_CHECKVERSION();
	auto context = ImGui::CreateContext();
	
	ImGuiIO& io = ImGui::GetIO();
	if( options.isKeyboardEnabled() ) io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	if( options.isGamepadEnabled() ) io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls	
	if (options.isDockingEnabled()) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	if (options.isViewportsEnabled()) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ci::app::WindowRef window = options.getWindow();
	io.DisplaySize = ci::vec2( window->toPixels( window->getSize() ) );
	io.DeltaTime = 1.0f / 60.0f;
	io.WantCaptureMouse = true;

	static std::string path;
	if( options.getIniPath().empty() ) {
		path = ( ci::app::getAssetPath( "" ) / "imgui.ini" ).string();
	}
	else {
		path = options.getIniPath().string().c_str();
	}
	io.IniFilename = path.c_str();

#if ! defined( CINDER_GL_ES )
	ImGui_ImplOpenGL3_Init( "#version 150" );
#else
	ImGui_ImplOpenGL3_Init();
#endif
	
	ImGui_ImplCinder_Init( window, options );
	if( options.isAutoRenderEnabled() ) {
		ImGui_ImplCinder_NewFrameGuard( window );
		sTriggerNewFrame = true;
	}
	
	sAppConnections += ci::app::App::get()->getSignalCleanup().connect( [context]() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplCinder_Shutdown();
		ImGui::DestroyContext( context );
	} );

	sInitialized = true;
	return sInitialized;
}

void ImGui::NewFrameGuard() {
	ImGui_ImplCinder_NewFrameGuard(ci::app::getWindow());
}