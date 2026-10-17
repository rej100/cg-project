#pragma once
#include "disable_all_warnings.h"
#include "opengl_includes.h"
// Suppress warnings in third-party code.
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <functional>
#include <optional>
#include <string_view>
#include <vector>
#include <filesystem>

enum class OpenGLVersion {
	GL2,
	GL3,
	GL41,
	GL45
};

class Window {
public:
	Window(std::string_view title, const glm::ivec2& windowSize, OpenGLVersion glVersion, bool presentable=true, bool visible=true);
	~Window();

	void close(); // Set shouldClose() to true.
	[[nodiscard]] bool shouldClose(); // Whether window should close (close() was called or user clicked the close button).
	void maximize();
	[[nodiscard]] bool isMinimized() const;

	// Apply font/style changes between frames, including changes requested by the UI.
	void setUiScale(float scale);
	[[nodiscard]] float getUiScale() const;

	void updateInput();
	void swapBuffers(); // Swap the front/back buffer


	void renderToImage(const std::filesystem::path& filePath, const bool flipY = false); // renders the output to an image

	using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
	void registerKeyCallback(KeyCallback&&);
	using CharCallback = std::function<void(unsigned unicodeCodePoint)>;
	void registerCharCallback(CharCallback&&);
	using MouseButtonCallback = std::function<void(int button, int action, int mods)>;
	void registerMouseButtonCallback(MouseButtonCallback&&);
	using MouseMoveCallback = std::function<void(const glm::vec2& cursorPos)>;
	void registerMouseMoveCallback(MouseMoveCallback&&);
	using ScrollCallback = std::function<void(const glm::vec2& offset)>;
	void registerScrollCallback(ScrollCallback&&);
	using WindowResizeCallback = std::function<void(const glm::ivec2& size)>;
	void registerWindowResizeCallback(WindowResizeCallback&&);

	bool isKeyPressed(int key) const;
	bool isMouseButtonPressed(int button) const;

	// NOTE: coordinates are such that the origin is at the left bottom of the screen.
	glm::vec2 getCursorPos() const; // DPI independent.
	glm::vec2 getNormalizedCursorPos() const; // Ranges from 0 to 1.
	glm::vec2 getCursorPixel() const;

	// Hides mouse and prevents it from going out of the window.
	// Useful for a first person camera.
	void setMouseCapture(bool capture);

	[[nodiscard]] glm::ivec2 getWindowSize() const;
	[[nodiscard]] glm::ivec2 getFrameBufferSize() const;
	[[nodiscard]] float getAspectRatio() const;
	[[nodiscard]] float getDpiScalingFactor() const;

private:
	void applyUiScale();
	static void contentScaleCallback(GLFWwindow* window, float xScale, float yScale);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void charCallback(GLFWwindow* window, unsigned unicodeCodePoint);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void windowSizeCallback(GLFWwindow* window, int width, int height);

private:
	GLFWwindow* m_pWindow;
	glm::ivec2 m_windowSize;
	float m_dpiScalingFactor = 1.0f;
	float m_uiScale = 1.0f;
	bool m_uiScaleDirty = false;
	ImGuiStyle m_unscaledStyle;
	const OpenGLVersion m_glVersion;
        bool m_presentable;

	std::vector<KeyCallback> m_keyCallbacks;
	std::vector<CharCallback> m_charCallbacks;
	std::vector<MouseButtonCallback> m_mouseButtonCallbacks;
	std::vector<ScrollCallback> m_scrollCallbacks;
	std::vector<MouseMoveCallback> m_mouseMoveCallbacks;
	std::vector<WindowResizeCallback> m_windowResizeCallbacks;
};
