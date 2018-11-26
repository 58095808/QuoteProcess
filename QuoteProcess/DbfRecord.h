#pragma once


#define SHDBFRECORDSIZE	265
#define SZDBFRECORDSIZE	352

#define FIELD_COUNT				27
#define INDEX_FIELD_COUNT		7

#define	PRECLOSE		0		//������
#define	OPEN		1		//����
#define	PRICE		2		//�ɽ�
#define	HIGH		3		//���
#define	LOW			4		//���
#define	VOLUMN		5		//����
#define	AMOUNT		6		//�ɽ���

#define	PBUY1		7		//����
#define	VBUY1		8		//����һ
#define	PSELL1		9		//����
#define	VSELL1		10		//����һ
#define	PBUY2		11		//��۶�
#define	VBUY2		12		//������
#define	PSELL2		13		//���۶�
#define	VSELL2		14		//������
#define	PBUY3		15		//�����
#define	VBUY3		16		//������
#define	PSELL3		17		//������
#define	VSELL3		18		//������
#define	PBUY4		19		//�����
#define	VBUY4		20		//������
#define	PBUY5		21		//�����
#define	VBUY5		22		//������
#define	PSELL4		23		//������
#define	VSELL4		24		//������
#define	PSELL5		25		//������
#define	VSELL5		26		//������


class CDbfRecord
{
public:
	CDbfRecord();
	~CDbfRecord();


	void Initial();

	long GetID();
	bool IsIndex();			// �Ƿ��Ǵ���ָ��
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

