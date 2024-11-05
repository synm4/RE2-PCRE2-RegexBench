// RealTimeTrackDlg.h : header file
//

#pragma once

#include "ChartViewer.h"
#include <afxmt.h>
#include <vector>


// CRealTimeTrackDlg dialog
class CRealTimeTrackDlg : public CDialog
{
// Construction
public:
	CRealTimeTrackDlg(CWnd* pParent = NULL);	// standard constructor
	~CRealTimeTrackDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REALTIMETRACK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	DECLARE_MESSAGE_MAP()



protected:
	// Controls
	CButton	m_RunPB;
	CStatic	m_ValueA;
	CStatic	m_ValueB;
	CStatic	m_ValueC;
	CComboBox	m_UpdatePeriod;
	CChartViewer m_ChartViewer;

	// Overrides
	virtual BOOL OnInitDialog();

	// Generated message map functions
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnRunPB();
	afx_msg void OnFreezePB();
	afx_msg void OnSelchangeUpdatePeriod();
	afx_msg void OnViewPortChanged();
	afx_msg void OnMouseMovePlotArea();

	// Storage for the realtime data
	std::vector<double> m_timeStamps;	// The timestamps for the data series
	std::vector<double> m_dataSeriesA;	// The values for the data series A
	std::vector<double> m_dataSeriesB;	// The values for the data series B
	std::vector<double> m_dataSeriesC;	// The values for the data series C

    // The index of the array position to which new data values are added.
    int m_currentIndex;

	double m_nextDataTime;	// Used by the random number generator to generate real time data.
	int m_extBgColor;		// The default background color.

	// Shift new data values into the real time data series 
	void getData();
	
	// Draw chart
	void drawChart(CChartViewer *viewer);
	void trackLineLegend(XYChart *c, int mouseX);

	// utility to get default background color
	int getDefaultBgColor();
	// utility to load icon resource to a button
	void loadButtonIcon(int buttonId, int iconId, int width, int height);
	// Convert vector to DoubleArray
	DoubleArray vectorToArray(const std::vector<double>& v, int startIndex = 0, int length = -1);
};
