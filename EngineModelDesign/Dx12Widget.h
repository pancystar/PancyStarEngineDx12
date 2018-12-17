#pragma once
#include <QWidget>
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
private:
	SceneRoot   *new_scene;
	virtual void paintEvent(QPaintEvent *event);   //���ڻ��ƺ���������render��ά����
private:
};

