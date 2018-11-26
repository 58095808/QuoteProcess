#pragma once


#define SHDBFRECORDSIZE	265
#define SZDBFRECORDSIZE	352

#define FIELD_COUNT				27
#define INDEX_FIELD_COUNT		7

#define	PRECLOSE		0		//昨收盘
#define	OPEN		1		//开盘
#define	PRICE		2		//成交
#define	HIGH		3		//最高
#define	LOW			4		//最低
#define	VOLUMN		5		//总手
#define	AMOUNT		6		//成交额

#define	PBUY1		7		//买入
#define	VBUY1		8		//买量一
#define	PSELL1		9		//卖出
#define	VSELL1		10		//卖量一
#define	PBUY2		11		//买价二
#define	VBUY2		12		//买量二
#define	PSELL2		13		//卖价二
#define	VSELL2		14		//卖量二
#define	PBUY3		15		//买价三
#define	VBUY3		16		//买量三
#define	PSELL3		17		//卖价三
#define	VSELL3		18		//卖量三
#define	PBUY4		19		//买价四
#define	VBUY4		20		//买量四
#define	PBUY5		21		//买价五
#define	VBUY5		22		//买量五
#define	PSELL4		23		//卖价四
#define	VSELL4		24		//卖量四
#define	PSELL5		25		//卖价五
#define	VSELL5		26		//卖量五


class CDbfRecord
{
public:
	CDbfRecord();
	~CDbfRecord();


	void Initial();

	long GetID();
	bool IsIndex();			// 是否是大盘指数
	void ReadDbfRecordSH(char*);
	void ReadDbfRecordSz(char*);

public:
	CRITICAL_SECTION m_rCritical;

	char m_cSecurityMarket;  
	long m_lSecurityCode;
	char m_pcName[9];
	double m_plRecordValues[FIELD_COUNT];

	long m_lQuoteDate;
	long m_lQuoteTime;
private:
	char m_pcBuffer[360];
};

