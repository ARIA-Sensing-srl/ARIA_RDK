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

class LinearColorMap : public QwtLinearColorMap
{
public:
	LinearColorMap( int formatType )
		: QwtLinearColorMap( Qt::darkCyan, Qt::red )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		addColorStop( 0.1, Qt::cyan );
		addColorStop( 0.6, Qt::green );
		addColorStop( 0.95, Qt::yellow );
	}
};

class HueColorMap : public QwtHueColorMap
{
public:
	HueColorMap( int formatType )
		: QwtHueColorMap( QwtColorMap::Indexed )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		//setHueInterval( 240, 60 );
		//setHueInterval( 240, 420 );
		setHueInterval( 0, 359 );
		setSaturation( 150 );
		setValue( 200 );
	}
};

class SaturationColorMap : public QwtSaturationValueColorMap
{
public:
	SaturationColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		setHue( 220 );
		setSaturationInterval( 0, 255 );
		setValueInterval( 255, 255 );
	}
};

class ValueColorMap : public QwtSaturationValueColorMap
{
public:
	ValueColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		setHue( 220 );
		setSaturationInterval( 255, 255 );
		setValueInterval( 70, 255 );
	}
};

class SVColorMap : public QwtSaturationValueColorMap
{
public:
	SVColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		setHue( 220 );
		setSaturationInterval( 100, 255 );
		setValueInterval( 70, 255 );
	}
};

class AlphaColorMap : public QwtAlphaColorMap
{
public:
	AlphaColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		//setColor( QColor("DarkSalmon") );
		setColor( QColor("SteelBlue") );
	}
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

	~QwtImageSCPlot()
	{}

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

	QwtDensityPlot( QWidget* = NULL, plotData_Density* data=nullptr);
	~QwtDensityPlot();
	void		updateContourValues();
	void		updateData(plotData_Density* data);
	void        updateZBar();
	void		forceRedraw() {m_spectrogram->invalidateCache();}

Q_SIGNALS:
	void rendered( const QString& status );

public Q_SLOTS:
	void showContour( bool on );
	void showSpectrogram( bool on );

	void setColorMap( int );
	void setColorTableSize( int );
	void setAlpha( int );

#ifndef WIN32
	void printPlot();
#endif

private:
	virtual void drawItems( QPainter*, const QRectF&,
						   const QwtScaleMap maps[QwtAxis::AxisPositions] ) const QWT_OVERRIDE;

	QwtImageSCPlot* m_spectrogram;

	int m_mapType;
	int m_alpha;
};


class dlgQWTPlot : public QDialog
{
	Q_OBJECT

public:
	explicit dlgQWTPlot(QWidget *parent = nullptr, class mdiOctaveInterface* owner = nullptr, octavews* ws = nullptr, PLOT_TYPE pt = PT_NONE);
	~dlgQWTPlot();

private:
	std::shared_ptr<plotData>	_plotdata;
	Ui::dlgQWTPlot				*ui;
	QwtPlotGrid					*_plotgrid;
	QwtPlotZoomer*				zoomer;
	QwtDensityPlot*				_density_plot_ref;
	class mdiOctaveInterface*	_owner;
public:
	PLOT_TYPE get_plot_type() {return _plotdata==nullptr ? PT_NONE : _plotdata->get_plot_type();}
	void assign_vars(QStringList xvars, QStringList yvars);
	void assign_vars(QString yname, QString xname="");
	void assign_vars(QString zname, QString yname, QString xname);
	bool has_var(QString vname);
	bool has_var(std::set<std::string>& varlist);
    void remove_var(QString varname);

public slots:
	void update_data(const std::set<std::string>& varlist);
	void update_workspace();
	void zoomAll();
	void cbContourChanged(Qt::CheckState state);
	void cbColorMapChanged(int index);
	void userZoom(const QRectF&);
};



#endif // DLGQWTPLOT_H
