#pragma once

class Main
{
public:

	static Main* GetSingleton()
	{
		static Main main;
		return &main;
	}

	void SetupLog();
	void Setup();
	void LoadINI();

};
