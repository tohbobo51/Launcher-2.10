#pragma once

class ButtonPanel : public Layout
{
public:
	ButtonPanel();
	void drawLauncherUi();
	bool handleLauncherTouch(int type, int pointer, int x, int y);

	CButton* m_bH;
private:
	OButton* m_bOpen = nullptr;
	int m_launcherMode = 0;
	int m_touchTarget = 0;
	int m_pressedAction = 0;
	int m_selectedWheelItem = -1;
	bool m_touchCaptured = false;
	bool m_phoneUnlocked = false;
	bool m_phoneSwipeArmed = false;
	float m_phoneSwipeOffset = 0.0f;
	ImVec2 m_touchStart = ImVec2(0.0f, 0.0f);
	std::string m_toastMessage;
	double m_toastUntil = 0.0;
	Button* m_bAlt;
	Button* m_bY;
	Button* m_bN;

};
