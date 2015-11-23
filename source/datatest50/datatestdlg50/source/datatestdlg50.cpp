/***********************************************
**  �ļ���:     datatestdlg50.cpp
**  ����:       �ɼ�����Ʋ��Գ������DN50��˫��ƽ��˫���ư�
**  ����ϵͳ:   ����Trolltech Qt4.8.5�Ŀ�ƽ̨ϵͳ
**  ����ʱ��:   2015/11/23
**  רҵ��:     ��³����������
**  ���������: YS
**  ����Ա:     YS
**  �汾��ʷ:   2015/11 ��һ��
**  ���ݰ���:
**  ˵��:		
**  ���¼�¼:   
***********************************************/

#include <QtGui/QMessageBox>
#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include "datatestdlg50.h"
#include "commondefine.h"
#include "parasetdlg.h"
#include "readcomconfig.h"
#include "algorithm.h"

DataTestDlg50::DataTestDlg50(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags)
{
	ui.setupUi(this);
	m_stdParam = new QSettings(getFullIniFileName("stdmtrparaset.ini"), QSettings::IniFormat);
}

DataTestDlg50::~DataTestDlg50()
{
}

void DataTestDlg50::closeEvent( QCloseEvent * event)
{
	qDebug()<<"^^^^^DataTestDlg::closeEvent";

// 	m_controlObj->askControlRelay(m_portsetinfo.waterOutNo, VALVE_OPEN); //�˳�ʱ�Զ��򿪷�ˮ��

	if (m_paraSetReader) //���춨����
	{
		delete m_paraSetReader;
		m_paraSetReader = NULL;
	}

	if (m_paraSetDlg) //�������öԻ���
	{
		delete m_paraSetDlg;
		m_paraSetDlg = NULL;
	}

	if (m_readComConfig)  //����������
	{
		delete m_readComConfig;
		m_readComConfig = NULL;
	}

	if (m_controlObj)  //���ſ���
	{
		closePump();//�˳�ʱ, �ر�ˮ��
		delete m_controlObj;
		m_controlObj = NULL;

		m_valveThread.exit();
	}

	if (m_controlObj2)  //���ſ���
	{
		delete m_controlObj2;
		m_controlObj2 = NULL;

		m_valveThread2.exit();
	}

	if (m_balanceObj)  //��ƽ�ɼ�
	{
		delete m_balanceObj;
		m_balanceObj = NULL;

		m_balanceThread.exit();
	}
	
	if (m_meterObj)  //����������ͨѶ
	{
		delete m_meterObj;
		m_meterObj = NULL;

		m_meterThread.exit();
	}
/*--------------------------------------------------------------*/
	if (m_tempTimer) //���¶ȼ�ʱ��
	{
		if (m_tempTimer->isActive())
		{
			m_tempTimer->stop();
		}
		delete m_tempTimer;
		m_tempTimer = NULL;
	}

	if (m_tempThread.isRunning())
	{
		m_tempThread.exit();
		if (m_tempObj)  // �¶Ȳɼ�
		{
			delete m_tempObj;
			m_tempObj = NULL;
		}
	}
/*--------------------------------------------------------------*/
	if (m_stdTempTimer) //����׼�¶ȼ�ʱ��
	{
		if (m_stdTempTimer->isActive())
		{
			m_stdTempTimer->stop();
		}
		delete m_stdTempTimer;
		m_stdTempTimer = NULL;
	}
	if (m_stdTempThread.isRunning())
	{
		m_stdTempThread.exit();
		if (m_stdTempObj)  // ��׼�¶Ȳɼ�
		{
			delete m_stdTempObj;
			m_stdTempObj = NULL;

			m_stdTempThread.exit();
		}
	}	
/*--------------------------------------------------------------*/
	if (m_flowRateTimer) //����������ʱ��
	{
		if (m_flowRateTimer->isActive())
		{
			m_flowRateTimer->stop();
		}
		delete m_flowRateTimer;
		m_flowRateTimer = NULL;
	}

	if (m_oldCoe)
	{
		delete m_oldCoe;
		m_oldCoe = NULL;
	}
/*****************************************************/
	if (m_instSTDMeterTimer)
	{
		if (m_instSTDMeterTimer->isActive())
		{
			m_instSTDMeterTimer->stop();
		}

		delete m_instSTDMeterTimer;
		m_instSTDMeterTimer = NULL;
	}

	if (m_instantFlowThread.isRunning())
	{
		m_instantFlowThread.exit();
		if (m_instantFlowCom)
		{
			delete m_instantFlowCom;
			m_instantFlowCom = NULL;
		}
	}	

	if (m_accumSTDMeterTimer)
	{
		if (m_accumSTDMeterTimer->isActive())
		{
			m_accumSTDMeterTimer->stop();
		}
		delete m_accumSTDMeterTimer;
		m_accumSTDMeterTimer = NULL;
	}

	if (m_accumFlowThread.isRunning())
	{
		m_accumFlowThread.exit();
		if (m_accumulateFlowCom)
		{
			delete m_accumulateFlowCom;
			m_accumulateFlowCom = NULL;
		}
	}
/*****************************************************/
	if (m_pidDataPtr)
	{
		delete m_pidDataPtr;
		m_pidDataPtr = NULL;
	}

	if (m_setRegularTimer) //�Զ�����������ʱ��
	{
		if (m_setRegularTimer->isActive())
		{
			m_setRegularTimer->stop();
		}
		delete m_setRegularTimer;
		m_setRegularTimer = NULL;
	}
	savePidParams();
/*****************************************************/
}

void DataTestDlg50::showEvent(QShowEvent *event)
{
	qDebug()<<"DataTestDlg showEvent thread:"<<QThread::currentThreadId();
	//��ȡ��λ���˿�������Ϣ
	if (!getPortSetIni(&m_portsetinfo))
	{
		QMessageBox::warning(this, tr("Warning"), tr("get port set info failed!"));//��ȡ��λ���˿�������Ϣʧ��!���������ã�
	}

	initValveStatus();      //��ʼ������״̬
	initRegulateStatus();   //��ʼ�����ڷ�״̬

	m_paraSetDlg = NULL;
	m_paraSetReader = new ParaSetReader(); //���������ýӿ�

	m_readComConfig = NULL;
	m_readComConfig = new ReadComConfig();
	m_readComConfig->getBalancePara(m_balMaxWht, m_balBottomWht); //��ȡ��ƽ��������ͻ�ˮ����

	m_tempObj = NULL;
	m_tempTimer = NULL;
	initTemperatureCom();	 //��ʼ����·�¶Ȳɼ�����

	m_stdTempObj = NULL;
	m_stdTempTimer = NULL;
	initStdTemperatureCom(); //��ʼ����׼�¶Ȳɼ�����

	m_controlObj = NULL;
	initControlCom();		//��ʼ�����ƴ���

	m_controlObj2 = NULL;
	initControlCom2();		//��ʼ�����ƴ���2

	m_balanceObj = NULL;
	initBalanceCom();		//��ʼ����ƽ����

	m_meterObj = NULL;
	initComOfHeatMeter();	//��ʼ������������

	//����������
	m_totalcount = 0;
	m_startWeight = 0.0;
	m_endWeight = 0.0;
	memset(m_deltaWeight, 0, sizeof(float)*FLOW_SAMPLE_NUM);
	m_flowRateTimer = new QTimer();
	connect(m_flowRateTimer, SIGNAL(timeout()), this, SLOT(slotFreshFlowRate()));
	m_flowRateTimer->start(TIMEOUT_FLOW_SAMPLE);

	m_oldCoe = new MeterCoe_STR;
	memset(m_oldCoe, 0, sizeof(MeterCoe_STR));

	/********************************************/
	m_instantFlowCom = NULL;
	m_instSTDMeterTimer = NULL;
	initInstStdCom();//��ʼ��˲ʱ��������

	m_accumulateFlowCom = NULL;
	m_accumSTDMeterTimer = NULL;
	initAccumStdCom();//��ʼ���ۻ���������
	/*********************************************/
}

//˲ʱ�����ɼ�����, ��λ��ֱ�Ӳɼ�
void DataTestDlg50::initInstStdCom()
{
	ComInfoStruct InstStdCom = m_readComConfig->ReadInstStdConfig();
	m_instantFlowCom = new lcModRtuComObject();
	m_instantFlowCom->moveToThread(&m_instantFlowThread);
	m_instantFlowThread.start();
	m_instantFlowCom->openLcModCom(&InstStdCom);

	connect(m_instantFlowCom, SIGNAL(lcModValueIsReady(const QByteArray &)), this, SLOT(slotGetInstStdMeterPulse(const QByteArray &)));

	if (m_instSTDMeterTimer == NULL)
		m_instSTDMeterTimer = new QTimer();

	connect(m_instSTDMeterTimer, SIGNAL(timeout()), this, SLOT(slotAskInstPulse()));
	m_instSTDMeterTimer->start(TIMEOUT_STD_INST);
}
//�ۼ������ɼ�����, ��λ��ֱ�Ӳɼ�
void DataTestDlg50::initAccumStdCom()
{
	ComInfoStruct AccumStdCom = m_readComConfig->ReadAccumStdConfig();
	m_accumulateFlowCom = new lcModRtuComObject();
	m_accumulateFlowCom->moveToThread(&m_accumFlowThread);
	m_accumFlowThread.start();
	m_accumulateFlowCom->openLcModCom(&AccumStdCom);

	connect(m_accumulateFlowCom, SIGNAL(lcModValueIsReady(const QByteArray &)), this, SLOT(slotGetAccumStdMeterPulse(const QByteArray &)));

	if (m_accumSTDMeterTimer == NULL)
		m_accumSTDMeterTimer = new QTimer();

	connect(m_accumSTDMeterTimer, SIGNAL(timeout()), this, SLOT(slotAskAccumPulse()));
	m_accumSTDMeterTimer->start(TIMEOUT_STD_ACCUM);
}

void DataTestDlg50::slotAskInstPulse()
{
	bool ok;
	uchar address = (uchar)m_stdParam->value("DevNo./InstDevNo").toString().toInt(&ok, 16);
	m_instantFlowCom->ask901712RoutesCmd(address);
}

void DataTestDlg50::slotAskAccumPulse()
{
	bool ok;
	uchar address = (uchar)m_stdParam->value("DevNo./AccumDevNo").toString().toInt(&ok, 16);
	m_accumulateFlowCom->ask9150A16RoutesCmd(address);
}

/*
** ����һ�����̣߳����¶Ȳɼ�����
*/
void DataTestDlg50::initTemperatureCom()
{
	ComInfoStruct tempStruct = m_readComConfig->ReadTempConfig();
	m_tempObj = new TempComObject();
	m_tempObj->moveToThread(&m_tempThread);
	m_tempThread.start();
	m_tempObj->openTemperatureCom(&tempStruct);
	connect(m_tempObj, SIGNAL(temperatureIsReady(const QString &)), this, SLOT(slotFreshComTempValue(const QString &)));

	m_tempTimer = new QTimer();
	//connect(m_tempTimer, SIGNAL(timeout()), m_tempObj, SLOT(writeTemperatureComBuffer()));
	connect(m_tempTimer, SIGNAL(timeout()), this, SLOT(slotAskPipeTemperature()));
// 	connect(m_tempTimer, SIGNAL(timeout()), this, SLOT(slotFreshFlow()));
	
	m_tempTimer->start(TIMEOUT_PIPE_TEMPER); //���������¶�
}

void DataTestDlg50::slotAskPipeTemperature()
{
	m_tempObj->writeTemperatureComBuffer();
}

/*
** ����һ�����̣߳��򿪱�׼�¶Ȳɼ�����
*/
void DataTestDlg50::initStdTemperatureCom()
{
	ComInfoStruct tempStruct = m_readComConfig->ReadStdTempConfig();
	m_stdTempObj = new StdTempComObject();
	QSettings stdconfig(getFullIniFileName("stdplasensor.ini"), QSettings::IniFormat);
	m_stdTempObj->moveToThread(&m_stdTempThread);
	m_stdTempThread.start();
	m_stdTempObj->openTemperatureCom(&tempStruct); //�ȴ򿪴��ڣ�Ȼ�������ñ�׼�¶ȼƵ�Э��汾
	m_stdTempObj->setStdTempVersion(stdconfig.value("in_use/model").toInt());
	connect(m_stdTempObj, SIGNAL(temperatureIsReady(const QString &)), this, SLOT(slotFreshStdTempValue(const QString &)));

	m_stdTempCommand = stdTempR1;
	m_stdTempTimer = new QTimer();
	connect(m_stdTempTimer, SIGNAL(timeout()), this, SLOT(slotAskStdTemperature()));
	
 	m_stdTempTimer->start(TIMEOUT_STD_TEMPER);
}

void DataTestDlg50::slotAskStdTemperature()
{
	m_stdTempObj->writeStdTempComBuffer(m_stdTempCommand);
}

//��ƽ�ɼ����� ��λ��ֱ�Ӳɼ�
void DataTestDlg50::initBalanceCom()
{
	ComInfoStruct balanceStruct = m_readComConfig->ReadBalanceConfig();
	m_balanceObj = new BalanceComObject();
	m_balanceObj->setBalanceType(m_readComConfig->getBalanceType());
	m_balanceObj->moveToThread(&m_balanceThread);
	m_balanceThread.start();
	m_balanceObj->openBalanceCom(&balanceStruct);

	//��ƽ��ֵ����λ��ֱ��ͨ����ƽ���ڲɼ�
	connect(m_balanceObj, SIGNAL(balanceValueIsReady(const float &)), this, SLOT(slotFreshBalanceValue(const float &)));
}

/*
** ����һ�����̣߳��򿪿��ư�ͨѶ����
*/
void DataTestDlg50::initControlCom()
{
	ComInfoStruct valveStruct = m_readComConfig->ReadValveConfig();
	m_controlObj = new ControlComObject();
	m_controlObj->setProtocolVersion(m_portsetinfo.version); //����Э��汾
	m_controlObj->moveToThread(&m_valveThread);
	m_valveThread.start();
	m_controlObj->openControlCom(&valveStruct);

	connect(m_controlObj, SIGNAL(controlRelayIsOk(const UINT8 &, const bool &)), this, SLOT(slotSetValveBtnStatus(const UINT8 &, const bool &)));
	connect(m_controlObj, SIGNAL(controlRegulateIsOk()), this, SLOT(slotSetRegulateOk()));
	 
	/*****************************************************************************************************/
	m_openRegulateTimes = 0;
	m_pidDataPtr = new PIDDataStr;
	m_pre_error = 0.0;
	m_integral = 0.0;
	m_maxRateGetted = false;
	ui.lnEditTargetRate->setReadOnly(false);
	installPidParams();
	m_setRegularTimer = new QTimer;
	connect(m_setRegularTimer, SIGNAL(timeout()), this, SLOT(slotSetRegulate()));
	/*****************************************************************************************************/
	//��ƽ��ֵ�ӿ��ư��ȡ
// 	connect(m_controlObj, SIGNAL(controlGetBalanceValueIsOk(const float&)), this, SLOT(slotFreshBalanceValue(const float &)));
}

/*
** ����һ�����̣߳��򿪿��ư�ͨѶ����2
*/
void DataTestDlg50::initControlCom2()
{
	ComInfoStruct valveStruct2 = m_readComConfig->ReadValveConfig2();
	m_controlObj2 = new ControlComObject();
	m_controlObj2->setProtocolVersion(m_portsetinfo.version); //����Э��汾
	m_controlObj2->moveToThread(&m_valveThread2);
	m_valveThread2.start();
	m_controlObj2->openControlCom(&valveStruct2);

	connect(m_controlObj2, SIGNAL(controlRelayIsOk(const UINT8 &, const bool &)), this, SLOT(slotSetValveBtnStatus(const UINT8 &, const bool &)));
	connect(m_controlObj2, SIGNAL(controlRegulateIsOk()), this, SLOT(slotSetRegulateOk()));
}

void DataTestDlg50::on_lnEditKp_returnPressed()
{
	QRegExp rx("\\d+(\\.\\d+)?");
	QString str = ui.lnEditKp->text();
	if (rx.exactMatch(str))
	{
		m_Kp = str.toFloat();
		if (m_setRegularTimer->isActive())
			m_setRegularTimer->stop();

		m_setRegularTimer->start(m_pickCycleTime);
		m_timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"); //��¼ʱ���
		qDebug()<<"m_Kp setted: " << m_Kp;
	}						 
}

void DataTestDlg50::on_lnEditKi_returnPressed()
{
	QRegExp rx("\\d+(\\.\\d+)?");
	QString str = ui.lnEditKi->text();
	if (rx.exactMatch(str))
	{
		m_Ki = str.toFloat();
		if (m_setRegularTimer->isActive())
			m_setRegularTimer->stop();

		m_setRegularTimer->start(m_pickCycleTime);
		m_timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"); //��¼ʱ���
		qDebug()<<"m_Ki setted: " << m_Ki;
	}
}

void DataTestDlg50::on_lnEditKd_returnPressed()
{
	QRegExp rx("\\d+(\\.\\d+)?");
	QString str = ui.lnEditKd->text();
	if (rx.exactMatch(str))
	{
		m_Kd = str.toFloat();
		if (m_setRegularTimer->isActive())
			m_setRegularTimer->stop();

		m_setRegularTimer->start(m_pickCycleTime);
		m_timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"); //��¼ʱ���
		qDebug()<<"m_Kd setted: " << m_Kd;
	}
}

void DataTestDlg50::on_lnEditCycleTime_returnPressed()
{
	QRegExp rx("\\d+(\\.\\d+)?");
	QString str = ui.lnEditCycleTime->text();
	if (rx.exactMatch(str))
	{
		m_pickCycleTime = str.toInt();
		qDebug()<<"m_pickCycleTime setted: " << m_pickCycleTime;
		if (m_setRegularTimer->isActive())
			m_setRegularTimer->stop();

		m_setRegularTimer->start(m_pickCycleTime);
		m_timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"); //��¼ʱ���
		qDebug()<<"m_setRegularTimer's Interval setted: " << m_pickCycleTime;
	}
}

void DataTestDlg50::savePidParams()
{
	QSettings pidConfig(getFullIniFileName("pidParameter.ini"), QSettings::IniFormat);

	pidConfig.setValue("Kp",ui.lnEditKp->text());
	pidConfig.setValue("Ki",ui.lnEditKi->text());
	pidConfig.setValue("Kd",ui.lnEditKd->text());
	pidConfig.setValue("CycleTime",ui.lnEditCycleTime->text());
}

void DataTestDlg50::installPidParams()
{
   QSettings pidConfig(getFullIniFileName("pidParameter.ini"), QSettings::IniFormat);

	ui.lnEditKp->setText(pidConfig.value("Kp").toString());
	ui.lnEditKi->setText(pidConfig.value("Ki").toString());
	ui.lnEditKd->setText(pidConfig.value("Kd").toString());
	ui.lnEditCycleTime->setText(pidConfig.value("CycleTime").toString());

	m_Kp = pidConfig.value("Kp").toFloat();
	m_Ki = pidConfig.value("Ki").toFloat();
	m_Kd = pidConfig.value("Kd").toFloat();
	m_pickCycleTime = pidConfig.value("CycleTime").toInt();
}

void DataTestDlg50::on_lnEditMaxRate_returnPressed()
{
	QString str = ui.lnEditMaxRate->text();
	QRegExp rx("\\d+(\\.\\d+)?");//ƥ��������С��
	if (rx.exactMatch(str))
	{
		m_maxRate = str.toFloat();
		m_maxRateGetted = true;
		qDebug()<<"max flow rate setted: " << m_maxRate;
		if (m_setRegularTimer->isActive())
			m_setRegularTimer->stop();
	}
}

void DataTestDlg50::on_lnEditTargetRate_returnPressed()
{						   
	if (!m_maxRateGetted)
	{
		QMessageBox::warning(this, tr("MaxRate"), tr("please set maxRate first!"));
		return;
	}

	if (!m_nowRegNo)
	{
		m_nowRegNo = m_portsetinfo.regflow1No;
	}

	QString str = ui.lnEditTargetRate->text();
	QRegExp rx("\\d+(\\.\\d+)?");//ƥ��������С��
	if (rx.exactMatch(str))
	{
		m_targetRate = str.toFloat();
		if (m_targetRate > m_maxRate)
		{
			QMessageBox::warning(this, tr("Too Large"), tr("setted rate is greater than maxRate, exit!"));
			return;
		}

		if (m_setRegularTimer->isActive())
			m_setRegularTimer->stop();

		qDebug() <<"$$$$$$$$$$$$$$$$$$$ starting m_setRegularTimer $$$$$$$$$$$$$$$$$$$";
		m_timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"); //��¼ʱ���
		openPump();//�趨Ŀ������, ��ˮ��
		slotSetRegulate();//�趨����������һ��
		m_setRegularTimer->start(m_pickCycleTime);
	}
}

void DataTestDlg50::slotSetRegulate()
{
	this->setRegulate(ui.lcdStdMeterFlowRate->value(), m_targetRate);
}

void DataTestDlg50::setRegulate(float currentRate, float targetRate)
{
	qDebug() << "+++++++++++++++++++++++++++++++++++++++++++++++++++";
	qDebug() << "current Rate : " << currentRate;
	qDebug() << "target Rate : " << targetRate;
	qDebug() << "current error : " << currentRate - targetRate;
	qDebug() << "current regular number: " << m_nowRegNo;
	qDebug() << "current waitting time: "  << m_pickCycleTime;

	float deltaV = qAbs(targetRate - currentRate);
	m_degree = degreeGet(currentRate, targetRate);
	qDebug() << "current degree: " << m_degree;
	m_controlObj->askControlRegulate(m_nowRegNo, m_degree);
	qDebug() << "%%%%%%% regular setted %%%%%%%";
	m_ifGainTargetRate = false;
	if (deltaV <= PRECISION)
	{
		qDebug() << "\n######################gain target rate######################n";
		m_ifGainTargetRate = true;
	}
	m_pidDataPtr->pid_timestamp    = m_timeStamp;
	m_pidDataPtr->pid_maxRate	   = m_maxRate;
	m_pidDataPtr->pid_currentRate  = currentRate;
	m_pidDataPtr->pid_targetRate   = targetRate;
	m_pidDataPtr->pid_currentError = currentRate - targetRate;
	m_pidDataPtr->pid_regularNo	   = m_nowRegNo;
	m_pidDataPtr->pid_waitTime	   = m_pickCycleTime;
	m_pidDataPtr->pid_currentDegree= m_degree;
	m_pidDataPtr->pid_gainTargetRate = 	m_ifGainTargetRate;
	m_pidDataPtr->pid_nowErrorPercent = ((currentRate - targetRate)/targetRate)*100;
	m_pidDataPtr->pid_pumpFreq = m_pumpFreq;
	insertPidRec(m_pidDataPtr);
}

int DataTestDlg50::degreeGet(float currentRate, float targetRate)
{
	m_curr_error = targetRate - currentRate;
	m_integral += m_curr_error*WAIT_SECOND;
	float derivative = (m_curr_error - m_pre_error)/WAIT_SECOND;
	float output = m_Kp*m_curr_error + m_Ki*m_integral + m_Kd*derivative;
	qDebug() << "Kp:--" <<m_Kp<<" Ki:--"<<m_Ki<<" Kd:--"<<m_Kd<<" maxRate:--"<<m_maxRate;
	qDebug() << "P:--" <<m_Kp*m_curr_error<<" I:--"<<m_Ki*m_integral<<" D:--"<<m_Kd*derivative;
	qDebug() << "oooooutput: " << output;
	m_pre_error = m_curr_error;
	int outdegree =  (int)(output>0 ? output: 0);
	if (outdegree > 99)
	{
		outdegree = 99;
	}
	m_pidDataPtr->pid_Kp = m_Kp;
	m_pidDataPtr->pid_Ki = m_Ki;
	m_pidDataPtr->pid_Kd = m_Kd;
	m_pidDataPtr->pid_P = m_Kp*m_curr_error;
	m_pidDataPtr->pid_I = m_Ki*m_integral;
	m_pidDataPtr->pid_D = m_Kd*derivative;

	return outdegree;
}

void DataTestDlg50::stopSetRegularTimer()
{
	if (m_setRegularTimer->isActive())
	{
		m_setRegularTimer->stop();
		qDebug() << "m_setRegularTimer stoped";
	}
}

//����������ͨѶ
void DataTestDlg50::initComOfHeatMeter()
{
	m_meterObj = new MeterComObject();
	int version = m_paraSetReader->getParams()->m_pickcode;
	m_meterObj->setProtocolVersion(version);//����Э��汾��
	m_meterObj->moveToThread(&m_meterThread);
	m_meterThread.start();

	connect(m_meterObj, SIGNAL(readMeterNoIsOK(const QString&, const QString&)), this, SLOT(slotFreshMeterNo(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterFlowIsOK(const QString&, const QString&)), this, SLOT(slotFreshMeterFlow(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterHeatIsOK(const QString&, const QString&)), this, SLOT(slotFreshMeterHeat(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterDateIsOK(const QString&, const QString&)), this, SLOT(slotFreshMeterDate(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterTemperIsOK(const QString&, const QString&, const QString&)), \
		this, SLOT(slotFreshMeterTemper(const QString&, const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterBigCoeIsOK(const QString&, const QString&)), \
		this, SLOT(slotFreshBigCoe(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterMid2CoeIsOK(const QString&, const QString&)), \
		this, SLOT(slotFreshMid2Coe(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterMid1CoeIsOK(const QString&, const QString&)), \
		this, SLOT(slotFreshMid1Coe(const QString&, const QString&)));
	connect(m_meterObj, SIGNAL(readMeterSmallCoeIsOK(const QString&, const QString&)), \
		this, SLOT(slotFreshSmallCoe(const QString&, const QString&)));

	QStringList cfgList = m_readComConfig->ReadIndexByName(meter(1));
	ui.portNameComboBox->setCurrentIndex(cfgList.at(0).toInt());
	ui.baudRateComboBox->setCurrentIndex(cfgList.at(1).toInt());
	ui.dataBitsComboBox->setCurrentIndex(cfgList.at(2).toInt());
	ui.parityComboBox->setCurrentIndex(cfgList.at(3).toInt());
	ui.stopBitsComboBox->setCurrentIndex(cfgList.at(4).toInt());

	ui.btnOpenCom->setEnabled(true);
	ui.btnCloseCom->setEnabled(false);
	setMeterComboxEnabled(true);
	setMeterOperBtnEnabled(false);
	clearMeterDispInfo();
}

//��ʼ������״̬
void DataTestDlg50::initValveStatus()
{
	m_nowPortNo = 0;

	//�˿ں�-���Ű�ť ӳ���ϵ
	m_valveBtn[m_portsetinfo.bigNo] = ui.btnValveBig;
	m_valveBtn[m_portsetinfo.smallNo] = ui.btnValveSmall;
	m_valveBtn[m_portsetinfo.middle1No] = ui.btnValveMiddle1;
	m_valveBtn[m_portsetinfo.middle2No] = ui.btnValveMiddle2;
	m_valveBtn[m_portsetinfo.waterInNo] = ui.btnWaterIn;
	m_valveBtn[m_portsetinfo.bigWaterInNo] = ui.btnBigWaterIn;
	m_valveBtn[m_portsetinfo.bigWaterOutNo] = ui.btnBigWaterOut;
	m_valveBtn[m_portsetinfo.smallWaterInNo] = ui.btnSmallWaterIn;
	m_valveBtn[m_portsetinfo.smallWaterOutNo] = ui.btnSmallWaterOut;
	m_valveBtn[m_portsetinfo.pumpNo] = ui.btnWaterPump; //ˮ��

	//�˿ں�-����״̬ ȫ������״̬Ϊ�ر�
	m_valveStatus[m_portsetinfo.bigNo] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.smallNo] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.middle1No] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.middle2No] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.waterInNo] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.bigWaterInNo] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.bigWaterOutNo] = VALVE_OPEN;
	m_valveStatus[m_portsetinfo.smallWaterInNo] = VALVE_CLOSE;
	m_valveStatus[m_portsetinfo.smallWaterOutNo] = VALVE_OPEN;
	//�˿ں�-ˮ��״̬ ��ʼΪ�ر�
	m_valveStatus[m_portsetinfo.pumpNo] = VALVE_CLOSE;

	setValveBtnBackColor(m_valveBtn[m_portsetinfo.bigNo], m_valveStatus[m_portsetinfo.bigNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.smallNo], m_valveStatus[m_portsetinfo.smallNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.middle1No], m_valveStatus[m_portsetinfo.middle1No]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.middle2No], m_valveStatus[m_portsetinfo.middle2No]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.waterInNo], m_valveStatus[m_portsetinfo.waterInNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.bigWaterInNo], m_valveStatus[m_portsetinfo.bigWaterInNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.bigWaterOutNo], m_valveStatus[m_portsetinfo.bigWaterOutNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.smallWaterInNo], m_valveStatus[m_portsetinfo.smallWaterInNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.smallWaterOutNo], m_valveStatus[m_portsetinfo.smallWaterOutNo]);
	setValveBtnBackColor(m_valveBtn[m_portsetinfo.pumpNo], m_valveStatus[m_portsetinfo.pumpNo]); //ˮ��
}

//��ʼ�����ڷ�״̬
void DataTestDlg50::initRegulateStatus()
{
	m_nowRegNo = 0;

	//�˿ں�-���ڷ���ť ӳ���ϵ
	m_regBtn[m_portsetinfo.regflow1No] = ui.btnRegulate1;

	//���ڷ���ʼ״̬
	m_regStatus[m_portsetinfo.regflow1No] = REG_SUCCESS;

	setRegBtnBackColor(m_regBtn[m_portsetinfo.regflow1No], m_regStatus[m_portsetinfo.regflow1No]);
}

//��������ͨѶ����
void DataTestDlg50::on_btnOpenCom_clicked()
{
	ComInfoStruct comStruct;
	comStruct.portName = ui.portNameComboBox->currentText();
	comStruct.baudRate = ui.baudRateComboBox->currentText().toInt();
	comStruct.dataBit = ui.dataBitsComboBox->currentText().toInt();
	comStruct.parity = ui.parityComboBox->currentIndex();
	comStruct.stopBit = ui.stopBitsComboBox->currentIndex();
	if (!m_meterObj->openMeterCom(&comStruct))
	{
		return; //���ȱ�ͨѶ����ʧ��
	}

	ui.btnOpenCom->setEnabled(false);
	ui.btnCloseCom->setEnabled(true);
	setMeterOperBtnEnabled(true);
	setMeterComboxEnabled(false);
}

//�ر�������ͨѶ����
void DataTestDlg50::on_btnCloseCom_clicked()
{
	m_meterObj->closeMeterCom();

	ui.btnOpenCom->setEnabled(true);
	ui.btnCloseCom->setEnabled(false);
	setMeterOperBtnEnabled(false);
	setMeterComboxEnabled(true);
}

/*
** ���Ƽ̵�������
*/
void DataTestDlg50::operateRelay()
{
	if (m_nowPortNo <= 8) //1#���ư�
	{
		m_controlObj->askControlRelay(m_nowPortNo, !m_valveStatus[m_nowPortNo]);
	}
	else //2#���ư�
	{
		m_controlObj2->askControlRelay(m_nowPortNo-8, !m_valveStatus[m_nowPortNo]);
	}

	if (m_portsetinfo.version == OLD_CTRL_VERSION) //�Ͽ��ư� �޷���
	{
		slotSetValveBtnStatus(m_nowPortNo, !m_valveStatus[m_nowPortNo]);
	}
}

void DataTestDlg50::on_btnWaterIn_clicked() //��ˮ��
{
	m_nowPortNo = m_portsetinfo.waterInNo;
	operateRelay();
}

void DataTestDlg50::on_btnBigWaterIn_clicked() //����ƽ��ˮ��
{
	m_nowPortNo = m_portsetinfo.bigWaterInNo;
	operateRelay();
}

void DataTestDlg50::on_btnBigWaterOut_clicked() //����ƽ��ˮ��
{
	m_nowPortNo = m_portsetinfo.bigWaterOutNo;
	operateRelay();
}

void DataTestDlg50::on_btnSmallWaterIn_clicked() //С��ƽ��ˮ��
{
	m_nowPortNo = m_portsetinfo.smallWaterInNo;
	operateRelay();
}

void DataTestDlg50::on_btnSmallWaterOut_clicked() //С��ƽ��ˮ��
{
	m_nowPortNo = m_portsetinfo.smallWaterOutNo;
	operateRelay();
}

void DataTestDlg50::on_btnValveBig_clicked() //��������
{
	m_nowPortNo = m_portsetinfo.bigNo;
	operateRelay();

	m_maxRateGetted = false;
	m_nowRegNo = m_portsetinfo.regflow1No;
}

void DataTestDlg50::on_btnValveMiddle1_clicked() //����һ��
{
	m_nowPortNo = m_portsetinfo.middle1No;
	operateRelay();

	m_maxRateGetted = false;
	m_nowRegNo = m_portsetinfo.regflow2No;
}

void DataTestDlg50::on_btnValveMiddle2_clicked() //��������
{
	m_nowPortNo = m_portsetinfo.middle2No;
	operateRelay();
}

void DataTestDlg50::on_btnValveSmall_clicked() //С������
{
	m_nowPortNo = m_portsetinfo.smallNo;
	operateRelay();
}

/*
** ����ˮ�ÿ���
*/
void DataTestDlg50::on_btnWaterPump_clicked() //ˮ��
{
	m_nowPortNo = m_portsetinfo.pumpNo;
	m_controlObj->askControlWaterPump(m_nowPortNo, !m_valveStatus[m_nowPortNo]);

	if (m_portsetinfo.version == OLD_CTRL_VERSION) //�Ͽ��ư� �޷���
	{
		slotSetValveBtnStatus(m_nowPortNo, !m_valveStatus[m_nowPortNo]);
	}
}

void DataTestDlg50::openPump()
{
	m_nowPortNo = m_portsetinfo.pumpNo;
	m_controlObj->askControlWaterPump(m_nowPortNo, true);

	if (m_portsetinfo.version == OLD_CTRL_VERSION) //�Ͽ��ư� �޷���
	{
		slotSetValveBtnStatus(m_nowPortNo, true);
	}
}

void DataTestDlg50::closePump()
{
	m_nowPortNo = m_portsetinfo.pumpNo;
	m_controlObj->askControlWaterPump(m_nowPortNo, false);

	if (m_portsetinfo.version == OLD_CTRL_VERSION) //�Ͽ��ư� �޷���
	{
		slotSetValveBtnStatus(m_nowPortNo, false);
	}
}

//����Ƶ��
void DataTestDlg50::on_btnSetFreq_clicked()
{
	m_pumpFreq = ui.spinBoxFreq->value();
	m_controlObj->askSetDriverFreq(ui.spinBoxFreq->value());
}

//���ڷ�
void DataTestDlg50::on_btnRegulate1_clicked() //���ڷ�1
{
	stopSetRegularTimer();
	m_nowRegNo = m_portsetinfo.regflow1No;
	setRegBtnBackColor(m_regBtn[m_nowRegNo], false); //��ʼ�����ڷ�����ɫ
	m_controlObj->askControlRegulate(m_nowRegNo, ui.spinBoxValveOpening->value());
}

void DataTestDlg50::on_btnRegulate2_clicked() //���ڷ�2
{
	stopSetRegularTimer();
	m_nowRegNo = m_portsetinfo.regflow2No;
	setRegBtnBackColor(m_regBtn[m_nowRegNo], false); //��ʼ�����ڷ�����ɫ
	m_controlObj->askControlRegulate(m_nowRegNo, ui.spinBoxValveOpening2->value());
}

//��������
void DataTestDlg50::on_btnParaSet_clicked()
{
	if (NULL == m_paraSetDlg)
	{
		m_paraSetDlg = new ParaSetDlg();
	}
	else
	{
		delete m_paraSetDlg;
		m_paraSetDlg = new ParaSetDlg();
	}
	m_paraSetDlg->show();
}

void DataTestDlg50::on_btnExit_clicked()
{
	this->close();
}

//�ɼ���׼�¶�
void DataTestDlg50::on_btnStdTempCollect_clicked()
{
	m_stdTempTimer->start(TIMEOUT_STD_TEMPER);
}

//ֹͣ�ɼ���׼�¶�
void DataTestDlg50::on_btnStdTempStop_clicked()
{
	m_stdTempTimer->stop();
}

void DataTestDlg50::on_lnEditInStdResist_textChanged(const QString & text)
{
	float resis = text.toFloat();
	float temp = calcTemperByResis(resis);
	ui.lnEditInStdTemp->setText(QString::number(temp));
}

void DataTestDlg50::on_lnEditOutStdResist_textChanged(const QString & text)
{
	float resis = text.toFloat();
	float temp = calcTemperByResis(resis);
	ui.lnEditOutStdTemp->setText(QString::number(temp));
}

//ˢ���¶�
void DataTestDlg50::slotFreshComTempValue(const QString& tempStr)
{
	ui.lnEditTempIn->setText(tempStr.left(TEMPER_DATA_WIDTH));   //����¶� PV
	ui.lnEditTempOut->setText(tempStr.right(TEMPER_DATA_WIDTH)); //�����¶� SV
}

//ˢ�±�׼�¶�
void DataTestDlg50::slotFreshStdTempValue(const QString& stdTempStr)
{
// 	qDebug()<<"stdTempStr ="<<stdTempStr<<"; m_stdTempCommand ="<<m_stdTempCommand;
	switch (m_stdTempCommand)
	{
	case stdTempR1: 
		ui.lnEditInStdResist->setText(stdTempStr);
		m_stdTempCommand = stdTempR2;
		break;
	case stdTempR2: 
		ui.lnEditOutStdResist->setText(stdTempStr);
		m_stdTempCommand = stdTempR1;
		break;
	default:
		break;
	}
}

//ˢ����ƽ��ֵ
void DataTestDlg50::slotFreshBalanceValue(const float& balValue)
{
	ui.lnEditBigBalance->setText(QString::number(balValue, 'f', 3));
	
	if (balValue > m_balMaxWht) //��ֹ��ƽ���
	{
		m_controlObj->askControlRelay(m_portsetinfo.waterOutNo, VALVE_OPEN);// �򿪷�ˮ��	
		m_controlObj->askControlRelay(m_portsetinfo.waterInNo, VALVE_CLOSE);// �رս�ˮ��
		if (m_portsetinfo.version == OLD_CTRL_VERSION) //�Ͽ��ư� �޷���
		{
			slotSetValveBtnStatus(m_portsetinfo.waterOutNo, VALVE_OPEN);
			slotSetValveBtnStatus(m_portsetinfo.waterInNo, VALVE_CLOSE);
		}
	}
}

//��Ӧ����״̬���óɹ�
void DataTestDlg50::slotSetValveBtnStatus(const UINT8 &portno, const bool &status)
{
	m_valveStatus[portno] = status;
	setValveBtnBackColor(m_valveBtn[portno], m_valveStatus[portno]);
}

//��Ӧ���ڷ����ڳɹ�
void DataTestDlg50::slotSetRegulateOk()
{
	setRegBtnBackColor(m_regBtn[m_nowRegNo], REG_SUCCESS);
}

//���÷��Ű�ť����ɫ
void DataTestDlg50::setValveBtnBackColor(QPushButton *btn, bool status)
{
	if (NULL == btn)
	{
		return;
	}
	if (status) //���Ŵ� ��ɫ
	{
		btn->setStyleSheet("background-color:rgb(0,255,0);"); 
	}
	else //���Źر� ��ɫ
	{
		btn->setStyleSheet("background-color:rgb(255,0,0);border:0px;border-image: url(:/datatestdlg/images/close.png);"); 
	}
}

//���õ��ڷ���ť����ɫ
void DataTestDlg50::setRegBtnBackColor(QPushButton *btn, bool status)
{
	if (NULL == btn)
	{
		return;
	}
	if (status) //���ڳɹ�
	{
// 		btn->setStyleSheet("background:rgb(0,255,0);border-image: url(:/datatestdlg/images/success.png);");
	}
	else //����ʧ��
	{
// 		btn->setStyleSheet("background:rgb(255,0,0);border-image: url(:/datatestdlg/images/failed.png);");
	}
}


/*
** ��������(ÿ1�����һ����ƽ�仯ֵ������ǰ10���ƽ������)
*/
void DataTestDlg50::slotFreshFlowRate()
{
	if (m_totalcount > 4294967290) //��ֹm_totalcount��� 32λ�޷���������Χ0~4294967295
	{
		m_totalcount = 0;
		m_startWeight = 0.0;
		m_endWeight = 0.0;
		memset(m_deltaWeight, 0, sizeof(float)*FLOW_SAMPLE_NUM);
	}
	if (m_totalcount == 0) //��¼��ƽ��ʼ����
	{
		m_startWeight = ui.lnEditBigBalance->text().replace(" ", 0).toFloat();
		m_totalcount ++;
		return;
	}

	float flowValue = 0.0;
	float totalWeight = 0.0;
	m_endWeight = ui.lnEditBigBalance->text().replace(" ", 0).toFloat();//ȡ��ǰ��ƽֵ, ��Ϊ��ǰ�������ֵ
	float delta_weight = m_endWeight - m_startWeight;
	m_deltaWeight[m_totalcount%FLOW_SAMPLE_NUM] = delta_weight;
// 	qWarning()<<"m_totalcount ="<<m_totalcount;
	for (int i=0; i<FLOW_SAMPLE_NUM; i++)
	{
		totalWeight += m_deltaWeight[i];
// 		qWarning()<<"totalWeight ="<<totalWeight<<", m_deltaWeight["<<i<<"] ="<<m_deltaWeight[i];
	}
	flowValue = 3.6*(totalWeight)*1000/(FLOW_SAMPLE_NUM*TIMEOUT_FLOW_SAMPLE);//���ۻ�ˮ��/��ʱ��  (��/Сʱ, t/h, m3/h)
//	flowValue = (totalWeight)*1000/(FLOW_SAMPLE_NUM*TIMEOUT_FLOW_SAMPLE);// kg/s
// 	qDebug()<<"flowValue ="<<flowValue;
	ui.lnEditFlowRate->setText(QString::number(flowValue, 'f', 3)); //��ui.lnEditFlowRate����ʾ����
	m_totalcount ++;//�������ۼ�
	m_startWeight = m_endWeight;//����ǰֵ����, ��Ϊ�´�����ĳ�ֵ
}

void DataTestDlg50::clearMeterDispInfo()
{
	ui.lnEditMeterNo->clear();
	ui.lnEditMeterTempIn->clear();
	ui.lnEditMeterTempOut->clear();
	ui.lnEditMeterTotalFlow->clear();
	ui.lnEditMeterHeat->clear();
	ui.dateEditMeter->setDate(QDate(2000,1,1));
	ui.lnEditBigOldError->clear();
	ui.lnEditBigOldCoe->clear();
	ui.lnEditBigOrgCoe->clear();
	ui.lnEditMid2OldError->clear();
	ui.lnEditMid2OldCoe->clear();
	ui.lnEditMid2OrgCoe->clear();
	ui.lnEditMid1OldError->clear();
	ui.lnEditMid1OldCoe->clear();
	ui.lnEditMid1OrgCoe->clear();
	ui.lnEditSmallOldError->clear();
	ui.lnEditSmallOldCoe->clear();
	ui.lnEditSmallOrgCoe->clear();
}

//���ü춨״̬
void DataTestDlg50::on_btnSetVerifyStatus_clicked()
{
	qDebug()<<"����춨״̬...";
	m_meterObj->askSetVerifyStatus(); //���ý���춨״̬
}

//�˳��춨״̬
void DataTestDlg50::on_btnExitVerifyStatus_clicked()
{
	qDebug()<<"�˳��춨״̬...";
	m_meterObj->askExitVerifyStatus(); //�����˳��춨״̬
}

//��������
void DataTestDlg50::on_btnReadMeterData_clicked()
{
	clearMeterDispInfo();
	qDebug()<<"���� ��ʼ...";
 	m_meterObj->askReadMeterData(); //�������
}

//�޸ı���
void DataTestDlg50::on_btnModifyMeterNo_clicked()
{
	QString oldMeterNo = ui.lnEditMeterNo->text();
	QString newMeterNo = ui.lnEditNewMeterNo->text();
	if (oldMeterNo.isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("MeterNO. is empty! please read meter first!"));
		return;
	}
	if (newMeterNo.isEmpty() || newMeterNo.size()!=2*CJ188_ADDR_LEN)
	{
		QMessageBox::warning(this, tr("Warning"), tr("new NO. is error!\nplease input 14 bits meter number!"));
		ui.lnEditNewMeterNo->setFocus();
		return;
	}

	m_meterObj->askModifyMeterNO(oldMeterNo, newMeterNo); //�����޸ı���
}

//�޸�����ϵ��
void DataTestDlg50::on_btnModifyFlowCoe_clicked()
{
	QString meterNO = ui.lnEditMeterNo->text();
	if (meterNO.isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("MeterNO. is empty! please read meter first!"));
		return;
	}
	bool ok;
	float bigErr = ui.lnEditBigNewError->text().toFloat(&ok);
	if (!ok || ui.lnEditBigNewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	float mid2Err = ui.lnEditMid2NewError->text().toFloat();
	if (!ok || ui.lnEditMid2NewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	float mid1Err = ui.lnEditMid1NewError->text().toFloat();
	if (!ok || ui.lnEditMid1NewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	float smallErr = ui.lnEditSmallNewError->text().toFloat();
	if (!ok || ui.lnEditSmallNewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	m_meterObj->askModifyFlowCoe(meterNO, bigErr, mid2Err, mid1Err, smallErr);
}

//�����޸�����ϵ��
void DataTestDlg50::on_btn2ModifyFlowCoe_clicked()
{
	QString meterNO = ui.lnEditMeterNo->text();
	if (meterNO.isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("MeterNO. is empty! please read meter first!"));
		return;
	}
	bool ok;
	float bigErr = ui.lnEditBigNewError->text().toFloat(&ok);
	if (!ok || ui.lnEditBigNewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	float mid2Err = ui.lnEditMid2NewError->text().toFloat(&ok);
	if (!ok || ui.lnEditMid2NewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	float mid1Err = ui.lnEditMid1NewError->text().toFloat(&ok);
	if (!ok || ui.lnEditMid1NewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}
	float smallErr = ui.lnEditSmallNewError->text().toFloat(&ok);
	if (!ok || ui.lnEditSmallNewError->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("err is error! please input err first!"));
		return;
	}

	m_oldCoe->bigCoe = ui.lnEditBigOldCoe->text().toFloat(&ok);
	if (!ok || ui.lnEditBigOldCoe->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("oldCoe is empty! please read meter first!"));
		return;
	}
	m_oldCoe->mid2Coe = ui.lnEditMid2OldCoe->text().toFloat(&ok);
	if (!ok || ui.lnEditMid2OldCoe->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("oldCoe is empty! please read meter first!"));
		return;
	}
	m_oldCoe->mid1Coe = ui.lnEditMid1OldCoe->text().toFloat(&ok);
	if (!ok || ui.lnEditMid1OldCoe->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("oldCoe is empty! please read meter first!"));
		return;
	}
	m_oldCoe->smallCoe = ui.lnEditSmallOldCoe->text().toFloat(&ok);
	if (!ok || ui.lnEditSmallOldCoe->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("oldCoe is empty! please read meter first!"));
		return;
	}
	m_meterObj->askModifyFlowCoe(meterNO, bigErr, mid2Err, mid1Err, smallErr, m_oldCoe);
}

//���ÿھ�
void DataTestDlg50::on_btnSetStandard_clicked()
{
	qDebug()<<"���ÿھ� ...";
	UINT8 std = ui.cmbStandard->currentIndex() + 1;
	m_meterObj->askSetStandard(std); //���ÿھ�
}

//����ϵͳʱ��
void DataTestDlg50::on_btnSetTime_clicked()
{
	qDebug()<<"����ϵͳʱ�� ...";
	m_meterObj->askSetSystemTime(); //����ϵͳʱ��
}

//����һ����ַ
void DataTestDlg50::on_btnSetAddr1_clicked()
{
	qDebug()<<"����һ����ַ ...";
	QString curAddr1 = ui.lnEditCurAddr1->text();
	QString newAddr1 = ui.lnEditAddress1->text();
	bool ok1;
	bool ok2;
	UINT8 curAddr = curAddr1.toUInt(&ok1, 10);
	UINT8 newAddr = newAddr1.toUInt(&ok2, 10);

	if (curAddr1.isEmpty() || !ok1)
	{
		QMessageBox::warning(this, tr("Warning"), tr("curAddr1 is error! please input 0-255!"));
		ui.lnEditCurAddr1->setFocus();
		return;
	}
	if (newAddr1.isEmpty() || !ok2)
	{
		QMessageBox::warning(this, tr("Warning"), tr("newAddr1 is error! please input 0-255!"));
		ui.lnEditAddress1->setFocus();
		return;
	}

	m_meterObj->askSetAddress1(curAddr1, newAddr1); //����һ����ַ
}

//���ö�����ַ
void DataTestDlg50::on_btnSetAddr2_clicked()
{
	qDebug()<<"���ö�����ַ ...";
	QString curAddr1 = ui.lnEditCurAddr1->text();
	QString newAddr2 = ui.lnEditAddress2->text();

	bool ok1;
	UINT8 curAddr = curAddr1.toUInt(&ok1, 10);
	if (curAddr1.isEmpty() || !ok1)
	{
		QMessageBox::warning(this, tr("Warning"), tr("curAddr1 is error! please input 0-255!"));
		ui.lnEditCurAddr1->setFocus();
		return;
	}

	m_meterObj->askSetAddress2(curAddr1, newAddr2); //���ö�����ַ
}

//��Ӧ��ȡ���ųɹ�
void DataTestDlg50::slotFreshMeterNo(const QString& comName, const QString& meterNo)
{
	ui.lnEditMeterNo->setText(meterNo);
	qDebug()<<"��ȡ���� �ɹ�...";
}

//��Ӧ��ȡ�������ɹ�
void DataTestDlg50::slotFreshMeterFlow(const QString& comName, const QString& flow)
{
	ui.lnEditMeterTotalFlow->setText(flow);
	qDebug()<<"��ȡ������ �ɹ�...";
}

//��Ӧ��ȡ�������ɹ�
void DataTestDlg50::slotFreshMeterHeat(const QString& comName, const QString& heat)
{
	ui.lnEditMeterHeat->setText(heat);
	qDebug()<<"��ȡ������ �ɹ�...";
}

//��Ӧ��ȡ�����ڳɹ�
void DataTestDlg50::slotFreshMeterDate(const QString& comName, const QString& date)
{
	ui.dateEditMeter->setDate(QDate::fromString(date, "yyyyMMdd"));
	qDebug()<<"��ȡ������ �ɹ�...";
}

//��Ӧ��ȡ������ˮ�¶ȳɹ�
void DataTestDlg50::slotFreshMeterTemper(const QString& comName, const QString& tempIn, const QString& tempOut)
{
	ui.lnEditMeterTempIn->setText(tempIn);
	ui.lnEditMeterTempOut->setText(tempOut);
	qDebug()<<"��ȡ������ˮ�¶� �ɹ�...";
}

void DataTestDlg50::slotFreshBigCoe(const QString& comName, const QString& bigCoe)
{
	ui.lnEditBigOrgCoe->setText(bigCoe);
	float bigCoeV = calcFloatValueOfCoe(bigCoe);
	ui.lnEditBigOldCoe->setText(QString::number(bigCoeV, 'f', COE_PRECISION));
	float bigErr = calcErrorValueOfCoe(bigCoe);
	ui.lnEditBigOldError->setText(QString::number(bigErr, 'f', COE_PRECISION));
	qDebug()<<"��ȡ������ϵ�� �ɹ�...";
}

void DataTestDlg50::slotFreshMid2Coe(const QString& comName, const QString& mid2Coe)
{
	ui.lnEditMid2OrgCoe->setText(mid2Coe);
	float mid2CoeV = calcFloatValueOfCoe(mid2Coe);
	ui.lnEditMid2OldCoe->setText(QString::number(mid2CoeV, 'f', COE_PRECISION));
	float mid2Err = calcErrorValueOfCoe(mid2Coe);
	ui.lnEditMid2OldError->setText(QString::number(mid2Err, 'f', COE_PRECISION));
	qDebug()<<"��ȡ����2����ϵ�� �ɹ�...";
}

void DataTestDlg50::slotFreshMid1Coe(const QString& comName, const QString& mid1Coe)
{
	ui.lnEditMid1OrgCoe->setText(mid1Coe);
	float mid1CoeV = calcFloatValueOfCoe(mid1Coe);
	ui.lnEditMid1OldCoe->setText(QString::number(mid1CoeV, 'f', COE_PRECISION));
	float mid1Err = calcErrorValueOfCoe(mid1Coe);
	ui.lnEditMid1OldError->setText(QString::number(mid1Err, 'f', COE_PRECISION));
	qDebug()<<"��ȡ����1����ϵ�� �ɹ�...";
}

void DataTestDlg50::slotFreshSmallCoe(const QString& comName, const QString& smallCoe)
{
	ui.lnEditSmallOrgCoe->setText(smallCoe);
	float smallCoeV = calcFloatValueOfCoe(smallCoe);
	ui.lnEditSmallOldCoe->setText(QString::number(smallCoeV, 'f', COE_PRECISION));
	float smallErr = calcErrorValueOfCoe(smallCoe);
	ui.lnEditSmallOldError->setText(QString::number(smallErr, 'f', COE_PRECISION));
	qDebug()<<"��ȡС����ϵ�� �ɹ�...";
}

void DataTestDlg50::setMeterComboxEnabled(bool flag)
{
	ui.portNameComboBox->setEnabled(flag);
	ui.baudRateComboBox->setEnabled(flag);
	ui.dataBitsComboBox->setEnabled(flag);
	ui.parityComboBox->setEnabled(flag);
	ui.stopBitsComboBox->setEnabled(flag);
}

void DataTestDlg50::setMeterOperBtnEnabled(bool flag)
{
	ui.btnSetVerifyStatus->setEnabled(flag);
	ui.btnExitVerifyStatus->setEnabled(flag);
	ui.btnReadMeterData->setEnabled(flag);
	ui.btnModifyMeterNo->setEnabled(flag);
	ui.btnModifyFlowCoe->setEnabled(flag);
	ui.btn2ModifyFlowCoe->setEnabled(flag);
	ui.btnSetStandard->setEnabled(flag);
	ui.cmbStandard->setEnabled(flag);
	ui.btnSetAddr1->setEnabled(flag);
	ui.btnSetAddr2->setEnabled(flag);
	ui.btnSetTime->setEnabled(flag);
}


void DataTestDlg50::slotGetInstStdMeterPulse(const QByteArray & valueArray)
{
	m_instStdPulse = valueArray;
	freshInstStdMeter();
	float instValue = 0.0;//˲ʱ����	
	instValue = ui.lcdInstStdMeter_3->value()+ui.lcdInstStdMeter_10->value()+ui.lcdInstStdMeter_25->value();
	ui.lcdStdMeterFlowRate->display(instValue);
}

int DataTestDlg50::getRouteByWdg(flow_rate_wdg wdgIdx, flow_type fType)
{
	int route = -1;
	m_stdParam->beginReadArray("Route");
	m_stdParam->setArrayIndex(wdgIdx);
	switch(fType)
	{
	case INST_FLOW_VALUE:
		route = m_stdParam->value("InstRoute").toInt();
		break;
	case ACCUM_FLOW_VALUE:
		route = m_stdParam->value("AccumRoute").toInt();
		break;
	default:
		break;
	}	
	m_stdParam->endArray();
	return route;
}

float DataTestDlg50::getStdUpperFlow(flow_rate_wdg wdgIdx)
{
	float upperFlow = 0.0f;

	m_stdParam->beginReadArray("Route");
	m_stdParam->setArrayIndex(wdgIdx);
	int diamWdg = m_stdParam->value("Diameter").toInt();//��ȡ�ܾ��Ĳ�����
	m_stdParam->endArray();

	m_stdParam->beginReadArray("FlowRate");
	m_stdParam->setArrayIndex(diamWdg);
	upperFlow = m_stdParam->value("UpperFlow").toFloat();
	m_stdParam->endArray();
	return upperFlow;
}

double DataTestDlg50::getStdPulse(flow_rate_wdg wdgIdx)
{
	double pulse = 0.0f;
	m_stdParam->beginReadArray("Route");
	m_stdParam->setArrayIndex(wdgIdx);
	int diamWdg = m_stdParam->value("Diameter").toInt();//��ȡ�ܾ��Ĳ�����
	m_stdParam->endArray();

	m_stdParam->beginReadArray("Pulse");
	m_stdParam->setArrayIndex(diamWdg);
	pulse = m_stdParam->value("Pulse").toDouble();
	m_stdParam->endArray();
	return pulse;
}

void DataTestDlg50::slotGetAccumStdMeterPulse(const QByteArray & valueArray)
{
	m_accumStdPulse = valueArray;
	freshAccumStdMeter();
	float accumValue = 0.0;//�ۻ�����	
	accumValue = ui.lcdAccumStdMeter_3->value()+ui.lcdAccumStdMeter_10->value()+ui.lcdAccumStdMeter_25->value();
	ui.lcdAccumStdMeter->display(accumValue);	
}

void DataTestDlg50::freshInstStdMeter()
{
	ui.lcdInstStdMeter_25->display(getInstFlowRate(FLOW_RATE_BIG));
	ui.lcdInstStdMeter_10->display(getInstFlowRate(FLOW_RATE_MID_1));
	ui.lcdInstStdMeter_3->display(getInstFlowRate(FLOW_RATE_SMALL));
}

void DataTestDlg50::freshAccumStdMeter()
{
	ui.lcdAccumStdMeter_25->display(getAccumFLowVolume(FLOW_RATE_BIG));
	ui.lcdAccumStdMeter_10->display(getAccumFLowVolume(FLOW_RATE_MID_1));
	ui.lcdAccumStdMeter_3->display(getAccumFLowVolume(FLOW_RATE_SMALL));
}

float DataTestDlg50::getInstFlowRate(flow_rate_wdg idx)
{
	int route = getRouteByWdg(idx, INST_FLOW_VALUE);
	int count = get9017RouteI(route, m_instStdPulse);
	float upperFlow = getStdUpperFlow(idx);
	//qDebug() <<"valve:--"<<idx<<" count:--"<<count<<" upperFlow:--"<<upperFlow;
	return getInstStdValue(count, upperFlow);
}

float DataTestDlg50::getAccumFLowVolume(flow_rate_wdg idx)
{
	int route = getRouteByWdg(idx, ACCUM_FLOW_VALUE);
	int count = get9150ARouteI(route, m_accumStdPulse);
	double pulse = getStdPulse(idx);
	//qDebug() <<"valve:--"<<idx<<" count:--"<<count<<" pulse:--"<<pulse;
	return count*pulse;
}