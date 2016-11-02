#ifndef HPDFWRITER_H
#define HPDFWRITER_H

/*
官方文档
https://github.com/libharu/libharu/wiki
*/

#include <QtCore>
#include <QtGui>
#include "./include/hpdf.h"

enum PDFTextAlign
{
	PDFAlign_Left,
	PDFAlign_Center,
	PDFAlign_Right
};

typedef struct PDFProperty
{
	PDFProperty()
	{
		titleSize	 = 30;
		contentSize  = 20;
		timeSize	 = 18;
		titleSpace	 = 20;
		lineSpace	 = 3;
		sectionSpace = 10;
		xedge		 = 30;
		yedge		 = 30;
	}
	int titleSize;		// 标题字体大小
	int contentSize;	// 内容字体大小
	int timeSize;		// 时间字体大小
	int titleSpace;		// 标题下空间
	int lineSpace;		// 行间距
	int sectionSpace;	// 段间距
	int xedge;			// 页左右边距
	int yedge;			// 页上下边距
} PDFProperty;

typedef struct PDFString
{
	PDFString(): Align(PDFAlign_Left) {}

	PDFString(PDFTextAlign align, const QString &text)
	{
		Align = align;
		Text  = text;
	}
	PDFString(const PDFString &other)
	{
		*this = other;
	}
	PDFString &operator=(const PDFString &other)
	{
		Align = other.Align;
		Text  = other.Text;
		return *this;
	}
	PDFTextAlign Align;
	QString   Text;
} PDFString;

typedef struct PDFItem
{
	PDFItem(){}
	PDFItem(const PDFString &title, const QList<PDFString> &sections)
	{
		Title = title;
		Sections = sections;
	}
	PDFItem(const PDFItem &other)
	{
		*this = other;
	}
	PDFItem &operator=(const PDFItem &other)
	{
		Title = other.Title;
		Sections = other.Sections;
		return *this;
	}
	PDFString Title;
	QList<PDFString>	Sections;
} PDFItem;

typedef QList<PDFItem> PDFContent;

class HPDFWriter
{
	Q_DECLARE_TR_FUNCTIONS(PDFWriter)

public:
	HPDFWriter()
	{
		initPDF();
		setPageSize(QSize(595, 842));
		setContentWidth(595 * 0.6);
		setPDFProperty(PDFProperty());
		initPDFFont();
	}

	void setPageSize(const QSize &size)
	{
		m_szPage = size;
	};

	void setContentWidth(int width)
	{
		m_wContent = min(width, m_szPage.width()) * ratio();
	}

	void setPDFProperty(PDFProperty property)
	{
		m_pro = property;
	}

	void initPDFFont();

	void setContent(const PDFContent &content)
	{
		m_mContent = content;
	}

	// LibHaru与Qt比例 Haru : Qt
	double ratio() const
	{
		return 1.247;
	}

	void saveToPDF(const QString &path);

	int result() const
	{
		return m_ret;
	}

private:
	void initPDF();
	std::string toLang(const QString &text) const;		// 转换到适合的语言的编码
	int  contentWidth(const HPDF_Page page, const QList<QString> &lines) const;
	void HPDF_Page_TextOutEx(HPDF_Page page, int edge, int ypos, PDFTextAlign align, const char *text, int width = 0);
	QList<QString> wrapString(const QString &text, const QFont &font, int width, int minWord);

private:
	int		m_ret;
	QSize	m_szPage;
	int		m_wContent;
	std::string  m_fName;
	std::string  m_codecName;

	PDFContent   m_mContent;
	PDFProperty	 m_pro;

	HPDF_Doc	 m_pdf;
	HPDF_Font	 m_font;
	HPDF_Encoder m_encoder;
};

#endif // HPDFWRITER_H