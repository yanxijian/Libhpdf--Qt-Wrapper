#include "HPDFWriter.h"
#pragma comment(lib, "./lib/libhpdf.lib")

// Get system font file path
std::string GetSystemFontFile(const std::wstring &faceName)
{
	static const LPWSTR fontRegistryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
	HKEY hKey;
	LONG result;

	// Open Windows font registry key
	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, fontRegistryPath, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS)
	{
		return "";
	}

	DWORD maxValueNameSize, maxValueDataSize;
	result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
	if (result != ERROR_SUCCESS)
	{
		return "";
	}

	DWORD valueIndex = 0;
	LPWSTR valueName = new WCHAR[maxValueNameSize];
	LPBYTE valueData = new BYTE[maxValueDataSize];
	DWORD valueNameSize, valueDataSize, valueType;
	std::wstring wsFontFile;

	// Look for a matching font name
	do
	{
		wsFontFile.clear();
		valueDataSize = maxValueDataSize;
		valueNameSize = maxValueNameSize;

		result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
		valueIndex++;

		if (result != ERROR_SUCCESS || valueType != REG_SZ)
		{
			continue;
		}

		std::wstring wsValueName(valueName, valueNameSize);

		// TTC may contain multiple fonts
		if (wsValueName.find(faceName) != std::wstring::npos)
		{
			// valueData may contain multiple fonts '\0', can't use assign
			wsFontFile = reinterpret_cast<LPWSTR>(valueData);
			break;
		}
	} while (result != ERROR_NO_MORE_ITEMS);

	delete[] valueName;
	delete[] valueData;

	RegCloseKey(hKey);

	if (wsFontFile.empty())
	{
		return "";
	}

	// Build full font file path
	WCHAR winDir[MAX_PATH];
	GetWindowsDirectory(winDir, MAX_PATH);

	std::wstringstream ss;
	ss << winDir << "\\Fonts\\" << wsFontFile;
	wsFontFile = ss.str();

	return std::string(wsFontFile.begin(), wsFontFile.end());
}

bool ends_with(std::string const & value, std::string const & ending)
{
	if (ending.size() > value.size()) return false;
	//return ending == value.substr(value.size() - ending.size());
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

jmp_buf env;

void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *)
{
	printf("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
		(HPDF_UINT)detail_no);
	longjmp(env, 1);
}

// Optional：https://github.com/libharu/libharu/wiki/Encodings
void HPDFWriter::initPDFFont()
{
	string codecName = "UTF-8";
	m_codecName = codecName;
	HPDF_UseUTFEncodings(m_pdf);
	
	// Get System Default Font
	NONCLIENTMETRICS im;
	im.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, im.cbSize, &im, 0);
	std::string fPath = GetSystemFontFile(im.lfMenuFont.lfFaceName);

	m_fName.clear();
	if (!fPath.empty())
	{
		fPath = strim(fPath);
		const std::string sttf(".ttf");
		const std::string sttc(".ttc");
		if (ends_with(fPath, sttf))		// ttf font
		{
			m_fName = HPDF_LoadTTFontFromFile(m_pdf, fPath.c_str(), HPDF_TRUE);
		}
		else if (ends_with(fPath, sttc))	// ttc font
		{
			m_fName = HPDF_LoadTTFontFromFile2(m_pdf, fPath.c_str(), 0, HPDF_TRUE);	// get firt font
		}
	}
	if (m_fName.empty())
	{
#ifndef ZHCN
		// set SimSun
		m_fName = "SimSun";
		codecName = "GBK-EUC-H";
		m_codecName = "GB18030";

		HPDF_UseCNSFonts(m_pdf);
		HPDF_UseCNSEncodings(m_pdf);
#else
		// set local font
		m_fName = HPDF_LoadTTFontFromFile(m_pdf, QString(qApp->applicationDirPath() + "/fonts/segoeui.ttf").toLocal8Bit().constData(), HPDF_TRUE);
#endif
	}

	HPDF_SetCurrentEncoder(m_pdf, codecName.c_str());
	m_encoder = HPDF_GetEncoder(m_pdf, codecName.c_str());
	m_font = HPDF_GetFont(m_pdf, m_fName.c_str(), codecName.c_str());
}

void HPDFWriter::saveToPDF(const QString& path)
{
	HPDF_Outline root;
	HPDF_Destination dst;

	/* Create bookmark */
	root = HPDF_CreateOutline(m_pdf, NULL, toLang(tr("Bookmark")).c_str(), m_encoder);
	HPDF_Outline_SetOpened(root, HPDF_TRUE);

	int topSpace = m_pro.yedge;
	int width;
	// Print  paragraph
	foreach(const PDFItem &item, m_mContent)
	{
		/* Create page */
		HPDF_Page page = HPDF_AddPage(m_pdf);
		HPDF_Page_SetWidth(page, m_szPage.width());
		HPDF_Page_SetHeight(page, m_szPage.height());

		/* Create bookmarks */
		HPDF_Outline outline = HPDF_CreateOutline(m_pdf, root, toLang(item.Title.Text).c_str(), m_encoder);
		dst = HPDF_Page_CreateDestination(page);
		HPDF_Destination_SetXYZ(dst, 0, HPDF_Page_GetHeight(page), 1);
		HPDF_Outline_SetDestination(outline, dst);

		/* Begin text content */
		HPDF_Page_BeginText(page);

		/* Set page property：print title */
		HPDF_Page_SetFontAndSize(page, m_font, m_pro.titleSize);
		// Left bottom pos
		topSpace = m_pro.yedge + m_pro.titleSize;
		HPDF_Page_TextOutEx(page, m_pro.xedge, m_szPage.height() - topSpace, item.Title.Align, toLang(item.Title.Text).c_str());
		topSpace += m_pro.titleSpace + m_pro.contentSize;

		/* Set page property：print content */
		QFont qfont(m_fName.c_str(), m_pro.contentSize);		// Font for  paragraph
		HPDF_Page_SetFontAndSize(page, m_font, m_pro.contentSize);
		bool mine = true;
		// Print  paragraph
		for (int cntContent = 0; cntContent < item.Sections.size(); ++cntContent)
		{
			// Print  paragraph：split into lines and print
			PDFString sContent = item.Sections.at(cntContent);
			QList<QString> lines = wrapString(sContent.Text, qfont, m_wContent, 10);
			width = contentWidth(page, lines);
			for (int cntLine = 0; cntLine < lines.size(); ++cntLine)
			{
				HPDF_Page_TextOutEx(page, m_pro.xedge, m_szPage.height() - topSpace, sContent.Align, toLang(lines.at(cntLine)).c_str(), width);

				// Not last line
				if (cntLine < lines.size() - 1)
				{
					// New line
					topSpace += m_pro.contentSize + m_pro.lineSpace;
					/* Out of page */
					if (topSpace >= m_szPage.height() - m_pro.yedge)
					{
						/* End current page */
						HPDF_Page_EndText(page);

						page = HPDF_AddPage(m_pdf);
						HPDF_Page_SetWidth(page, m_szPage.width());
						HPDF_Page_SetHeight(page, m_szPage.height());

						/* Begin new page */
						HPDF_Page_SetFontAndSize(page, m_font, m_pro.contentSize);
						HPDF_Page_BeginText(page);

						topSpace = m_pro.yedge + m_pro.contentSize;
					}
				}
			}
			mine = !mine;

			// New paragraph
			topSpace += m_pro.contentSize + m_pro.sectionSpace;
			/* Out of page */
			if (topSpace >= m_szPage.height() - m_pro.yedge)
			{
				/* End current page */
				HPDF_Page_EndText(page);

				page = HPDF_AddPage(m_pdf);
				HPDF_Page_SetWidth(page, m_szPage.width());
				HPDF_Page_SetHeight(page, m_szPage.height());

				/* Begin new page */
				HPDF_Page_BeginText(page);
				HPDF_Page_SetFontAndSize(page, m_font, m_pro.contentSize);

				topSpace = m_pro.yedge + m_pro.contentSize;
			}
		}

		/* End current page */
		HPDF_Page_EndText(page);
	}

	/* Save to PDF to file*/

	// HPDF_SaveToFile to a non-English path would crash
	QFile file(path);
	if (file.open(QIODevice::WriteOnly))
	{
		HPDF_SaveToStream(m_pdf);
		/* get the data from the stream and output it to stdout. */
		for (;;)
		{
			HPDF_BYTE buf[4096];
			HPDF_UINT32 siz = 4096;
			
			HPDF_ReadFromStream(m_pdf, buf, &siz);
			if (siz == 0)
			{
				break;
			}

			if (-1 == file.write(reinterpret_cast<const char *>(buf), siz))
			{
				qDebug() << "Message save as PDF error";
				break;
			}
		}
	}
	else
	{
		m_ret = -2;
	}
	/* Clean up*/
	HPDF_Free(m_pdf);
}

void HPDFWriter::initPDF()
{
	m_ret = -1;
	m_pdf = HPDF_New(error_handler, NULL);
	if (!m_pdf)
	{
		printf("error: cannot create PdfDoc object\n");
		return;
	}

	if (setjmp(env))
	{
		HPDF_Free(m_pdf);
	}
	else
	{
		/* Set page mode to use outlines */
		HPDF_SetPageMode(m_pdf, HPDF_PAGE_MODE_USE_OUTLINE);
		m_ret = 0;
	}
}

std::string HPDFWriter::toLang(const QString& text) const
{
	QTextCodec *codec = QTextCodec::codecForName(m_codecName.c_str());
	return codec->fromUnicode(text).constData();
}

int HPDFWriter::contentWidth(const HPDF_Page page, const QList<QString> &lines) const
{
	int width = 0;
	foreach(const QString &line, lines)
	{
		width = qMax(width, (int)HPDF_Page_TextWidth(page, toLang(line).c_str()));
	}
	return  width;
}

void HPDFWriter::HPDF_Page_TextOutEx(HPDF_Page page, int edge, int ypos, PDFTextAlign align, const char *text, int width)
{
	int pageWidth = HPDF_Page_GetWidth(page);

	if (0 == width && PDFAlign_Left != align)
	{
		width = HPDF_Page_TextWidth(page, text);
	}
	int xpos = edge;
	if (PDFAlign_Center == align)
	{
		xpos = (pageWidth - width) / 2;
	}
	else if (PDFAlign_Right == align)
	{
		xpos = pageWidth - width - edge;
	}
	// No line break. May be garbled
	if (!QRegExp("[\r\n]").exactMatch(text))
	{
		HPDF_Page_TextOut(page, xpos, ypos, text);
	}
}

QList<QString> HPDFWriter::wrapString(const QString& text, const QFont& font, int width, int minWord)
{
	QFontMetrics metrics(font);
	QList<QString> lText;
	int pos = 0;
	int n = minWord;
	while (pos < text.size())
	{
		n = minWord;
		if (pos + n >= text.size())
		{
			n = text.size() - pos;
			break;
		}
		while (metrics.width(text.mid(pos, n)) < width)
		{
			++n;
			if (pos + n >= text.size())
			{
				break;
			}
		}
		lText.append(text.mid(pos, n));
		pos += n;
	}
	if (n > 0 && pos < text.size())
	{
		lText.append(text.mid(pos, n));
	}
	return  lText;
}
