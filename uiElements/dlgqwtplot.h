#ifndef DLGQWTPLOT_H
#define DLGQWTPLOT_H

#include <QDialog>
#include "plotdescriptor.h"
#include <qwt_plot_curve.h>
#include <plotdata.h>
#include <QwtPlot>
#include <QwtPlotCurve>
#include <QwtPlotGrid>
#include <QwtSymbol>
#include <QwtLegend>

namespace Ui {
class dlgQWTPlot;
}

class QwtPlotSpectrogram;

class QwtDensityPlot : public QwtPlot
{
	Q_OBJECT

public:
	enum ColorMap
	{
		RGBMap,
		HueMap,
		SaturationMap,
		ValueMap,
		SVMap,
		AlphaMap
	};

	QwtDensityPlot( QWidget* = NULL );

Q_SIGNALS:
	void rendered( const QString& status );

public Q_SLOTS:
	void showContour( bool on );
	void showSpectrogram( bool on );

	void setColorMap( int );
	void setColorTableSize( int );
	void setAlpha( int );

#ifndef QT_NO_PRINTER
	void printPlot();
#endif

private:
	virtual void drawItems( QPainter*, const QRectF&,
						   const QwtScaleMap maps[QwtAxis::AxisPositions] ) const QWT_OVERRIDE;

	class QwtImageSCPlot* m_spectrogram;

	int m_mapType;
	int m_alpha;
};

class MyZoomer : public QwtPlotZoomer
{
public:
	MyZoomer( QWidget* canvas )
		: QwtPlotZoomer( canvas )
	{
		setTrackerMode( AlwaysOn );
	}

	virtual QwtText trackerTextF( const QPointF& pos ) const QWT_OVERRIDE
	{
		QColor bg( Qt::white );
		bg.setAlpha( 200 );

		QwtText text = QwtPlotZoomer::trackerTextF( pos );
		text.setBackgroundBrush( QBrush( bg ) );
		return text;
	}
};


class dlgQWTPlot : public QDialog
{
	Q_OBJECT

public:
	explicit dlgQWTPlot(QWidget *parent = nullptr, octavews* ws = nullptr, PLOT_TYPE pt = PT_NONE);
	~dlgQWTPlot();

private:
	std::shared_ptr<plotData>	_plotdata;
	Ui::dlgQWTPlot *ui;
	QwtPlotGrid					*_plotgrid;
	QwtPlotZoomer*				zoomer;
	QwtDensityPlot*				_density_plot_ref;
public:
	PLOT_TYPE get_plot_type() {return _plotdata==nullptr ? PT_NONE : _plotdata->get_plot_type();}
	void assign_vars(QString yname, QString xname="");
	bool has_var(QString vname);
	bool has_var(std::set<std::string>& varlist);

public slots:
	void update_data(const std::set<std::string>& varlist);
	void update_workspace();
	void zoomAll();
	void cbContourChanged(Qt::CheckState state);
};


class QwtImageSCPlot : public QwtPlotSpectrogram
{
public:
	int elapsed() const
	{
		return m_elapsed;
	}

	QSize renderedSize() const
	{
		return m_renderedSize;
	}

protected:
	virtual QImage renderImage(
		const QwtScaleMap& xMap, const QwtScaleMap& yMap,
		const QRectF& area, const QSize& imageSize ) const QWT_OVERRIDE
	{
		QImage image = QwtPlotSpectrogram::renderImage(
			xMap, yMap, area, imageSize );
		m_renderedSize = imageSize;

		return image;
	}

private:
	mutable int m_elapsed;
	mutable QSize m_renderedSize;
};
#endif // DLGQWTPLOT_H
