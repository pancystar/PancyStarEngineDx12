#include"PancyTimeBasic.h"
GlobelTimeCount::GlobelTimeCount()
{
	__int64 rec_frequency;
	if (QueryPerformanceFrequency((LARGE_INTEGER*)&rec_frequency))//��ȡϵͳ��ʱ��Ƶ��
	{
		count_freq = 1.0 / (double)rec_frequency;
	}
	if_stop = true;
	start_time = 0;
	all_pause_time = 0;
	stop_time = 0;
	now_time = 0;
	last_time = 0;
	delta_time = 0;
}
void GlobelTimeCount::Reset()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);//���ó�ʼʱ��
	if_stop = false;
	now_time = start_time;
	last_time = start_time;
}
void GlobelTimeCount::Start()//��ʼ��ʱ��ȡ����ͣ״̬������ͣʱ�����
{
	if (if_stop)
	{
		if_stop = false;
		all_pause_time = 0;
	}
}
void GlobelTimeCount::Stop()//��ͣ��ʱ��ˢ�¿�ʼ��ͣ��ʱ�̲���ʼ������ͣʱ��
{
	if (!if_stop)
	{
		Refresh();
		stop_time = now_time;
		delta_time = 0;
		if_stop = true;
	}
}
void GlobelTimeCount::Refresh()
{
	if (!if_stop)//ʱ��δ����ͣ������������ʱ
	{
		last_time = now_time;
		QueryPerformanceCounter((LARGE_INTEGER*)(&now_time));
		delta_time = static_cast<double>(now_time - last_time) * count_freq;
	}
	else//ʱ�䱻��ͣ�������ܵ���ͣʱ��
	{
		QueryPerformanceCounter((LARGE_INTEGER*)(&now_time));
		all_pause_time = static_cast<double>(now_time - stop_time) * count_freq;
	}
	all_timeneed = static_cast<double>(now_time - start_time) * count_freq;
}
float GlobelTimeCount::GetPause()
{
	return static_cast<float>(all_pause_time);
}
float GlobelTimeCount::GetDelta()
{
	return static_cast<float>(delta_time);
}
float GlobelTimeCount::GetAllTime()
{
	return static_cast<float>(all_timeneed);
}
__int64 GlobelTimeCount::GetSystemTime()
{
	__int64  system_time;      //��ʼʱ��
	QueryPerformanceCounter((LARGE_INTEGER*)(&system_time));
	return system_time;
}