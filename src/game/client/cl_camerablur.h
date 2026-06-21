#pragma once

class CViewSetup;

void CamBlur_Init();
void CamBlur_OnOverrideView(CViewSetup* pSetup);
void CamBlur_PostRender();
void CamBlur_Cycle();