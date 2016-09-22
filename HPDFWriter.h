#ifndef HPDFWRITER_H
#define HPDFWRITER_H

/*
Official Documentation
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

enum PDFLang
{
	PDFLang_SC,		// Simplified Chinese
	PDFLang_EN,		// English
	PDFLang_TC		// Tradition Chinese
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
	int titleSize;			// Title font size
	int contentSize;		// Content font size
	int timeSize;		// Time font size (mid content)
	int titleSpace;		// Space under title
	int lineSpace;		// Line space
	int sectionSpace;	// Paragraph space
	int xedge;			// Left and right margins
	int yedge;			// Top and bottom margins
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
		setPDFLang(PDFLang_SC);
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

	void setPDFLang(PDFLang lang);

	void setContent(const PDFContent &content)
	{
		m_mContent = content;
	}

	// LibHaru VS Qt font size ratio
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
	std::string toLang(const QString &text) const;		// Convert to proper encoding
	int  contentWidth(const HPDF_Page page, const QList<QString> &lines) const;
	void HPDF_Page_TextOutEx(HPDF_Page page, int edge, int ypos, PDFTextAlign align, const char *text, int width = 0);
	QList<QString> wrapString(const QString &text, const QFont &font, int width, int minWord);

private:
	int		m_ret;
	PDFLang m_lang;
	QSize	m_szPage;
	int		m_wContent;
	std::string  m_fName;

	PDFContent   m_mContent;
	PDFProperty	 m_pro;

	HPDF_Doc	 m_pdf;
	HPDF_Font	 m_font;
	HPDF_Encoder m_encoder;
};

#endif // HPDFWRITER_H