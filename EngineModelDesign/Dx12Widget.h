/*
https://blog.csdn.net/u012234115/article/details/47402277
*/
#pragma once
#include <QWidget>
#include <QMouseEvent>
#include"TestScene.h"
class D3d12RenderWidget : public QWidget
{
	Q_OBJECT
public:
	D3d12RenderWidget(QWidget *parent);
	~D3d12RenderWidget();
	virtual QPaintEngine *paintEngine() const
	{
		return NULL;
	}
	PancystarEngine::EngineFailReason Create(SceneRoot *new_scene_in);
	PancystarEngine::EngineFailReason LoadModel(std::string file_name);
	PancystarEngine::EngineFailReason ChangeModelSize(float scal_size);
	PancystarEngine::EngineFailReason ChangeModelPosition(float pos_x,float pos_y,float pos_z);
	PancystarEngine::EngineFailReason ChangeModelRotation(float rot_x, float rot_y, float rot_z);
private:
	bool if_build;
	SceneRoot   *new_scene;
	virtual void paintEvent(QPaintEvent *event);   //窗口绘制函数，用于render三维场景
	virtual void mouseDoubleClickEvent(QMouseEvent *event_need);
private:
};

