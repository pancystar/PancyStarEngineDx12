#pragma once
#include"PancystarEngineBasicDx12.h"
class GlobelTimeCount
{
	double   count_freq;      //ϵͳʱ��Ƶ��
	double   delta_time;      //��֮֡��ʱ���
	double   all_timeneed;    //������ʱ��
	double   all_pause_time;  //��ͣ����ʱ��
	__int64  start_time;      //��ʼʱ��
	__int64  stop_time;       //ֹͣʱ��
	__int64  now_time;        //��ǰʱ��
	__int64  last_time;       //��һ֡��ʱ��
	bool     if_stop;         //�Ƿ���ͣ
private:
	GlobelTimeCount();          //���캯��
public:
	static GlobelTimeCount* GetInstance()
	{
		static GlobelTimeCount* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new GlobelTimeCount();
		}
		return this_instance;
	}
	void Reset();          //ʱ������
	void Start();          //��ʼ��ʱ
	void Stop();           //��ͣ��ʱ
	void Refresh();        //ˢ�¼�ʱ��
	float GetPause();      //��ȡ�ܵ���ͣʱ��
	float GetDelta();      //��ȡ֡��ʱ���
	float GetAllTime();    //��ȡ��ʱ��
	__int64 GetSystemTime();//��ȡϵͳʱ��
	double GetSystemFreq()
	{
		return count_freq;
	};//��ȡϵͳÿ΢��ʱ����

};