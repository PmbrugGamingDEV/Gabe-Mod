Resource/GameLogo.res
{
	GameLogo
	{
		ControlName	EditablePanel
		fieldName	GameLogo
		xpos		0
		ypos		0
		zpos		0
		wide		256 // Right - left
		tall		128 // Bottom - top
		autoResize	1
		pinCorner	0
		visible		1
		enabled		1
		offsetX		0 // X-axis position
		offsetY		0 // Y-axis position
	}

	Logo
	{
		ControlName	ImagePanel
		fieldName	Logo
		xpos		0   // -Left
		ypos		-64 // -Top
		zpos		0
		wide		256 // Scaled VTF width
		tall		256 // Scaled VTF height
		visible		1
		enabled		1
		image		ui_logo
		scaleImage	1		
	}
}