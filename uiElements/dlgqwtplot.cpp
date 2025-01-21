#include "dlgqwtplot.h"
#include "ui_dlgqwtplot.h"
#include <QwtPlot>
#include <QwtPlotCurve>
#include <QwtPlotGrid>
#include <QwtSymbol>
#include <QwtLegend>
#include <qwt_scale_engine.h>
#include <QPrinter>
#include <QPrintDialog>



dlgQWTPlot::dlgQWTPlot(QWidget *parent, octavews* ws, PLOT_TYPE pt)
	: QDialog(parent)
	, ui(new Ui::dlgQWTPlot)
{
	ui->setupUi(this);
	zoomer = nullptr;
	_density_plot_ref = nullptr;


	switch(pt)
	{
	case PTQWT_PLOT:
	case PTQWT_SCATTER:
	{
		_plotdata = std::make_shared<plotData_plot>(ws,ui->plot);
		_plotgrid = new QwtPlotGrid();
		ui->plot->setCanvasBackground( Qt::white );
		//ui->plot->setAxisScale( QwtAxis::YLeft, 0.0, 10.0 );
		ui->plot->insertLegend( new QwtLegend() );
		ui->plot->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating,true);
		ui->plot->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating,true);
		ui->plot->setAxisAutoScale(QwtAxis::YLeft, false);
		ui->plot->setAxisAutoScale(QwtAxis::XBottom, false);

		ui->plot->setAutoReplot(false);
		ui->plot->setTitle("Fast plot");

		_plotgrid->attach(ui->plot);
		_plotgrid->enableX(true);
		_plotgrid->enableY(true);
		_plotgrid->enableXMin(true);
		_plotgrid->enableYMin(true);

		ui->cbContour->setEnabled(false);
		ui->cbContour->setVisible(false);
		ui->cbColorMap->setEnabled(false);
		ui->cbColorMap->setVisible(false);
		break;
	}

	case PTQWT_DENSITY:
	{
		_plotdata = std::make_shared<plotData_Density>(ws);
		delete ui->plot;
		ui->plot =_density_plot_ref= new QwtDensityPlot(this,(plotData_Density*)_plotdata.get());
		ui->plot->setObjectName("plot");

		ui->gridLayout->addWidget(ui->plot, 1, 0, 1, 6);
		_density_plot_ref->replot();

		ui->cbContour->setEnabled(true);
		ui->cbContour->setVisible(true);
		ui->cbColorMap->setEnabled(true);
		ui->cbColorMap->setVisible(true);

		connect(ui->cbContour, &QCheckBox::checkStateChanged, this, &dlgQWTPlot::cbContourChanged);
		connect(ui->cbColorMap, &QComboBox::currentIndexChanged, this, &dlgQWTPlot::cbColorMapChanged);
		ui->plot->setAxisAutoScale(QwtPlot::xBottom,true);
		ui->plot->setAxisAutoScale(QwtPlot::yLeft,true);

		this->setUpdatesEnabled(true);

		_density_plot_ref->setAutoReplot(true);


		break;
	}
	default:
		_plotdata = std::make_shared<plotData>(ws, ui->plot);
	}


	zoomer = new MyZoomer( ui->plot->canvas() );
	zoomer->setMousePattern( QwtEventPattern::MouseSelect2,
							Qt::RightButton, Qt::ControlModifier );
	zoomer->setMousePattern( QwtEventPattern::MouseSelect3,
							Qt::RightButton );

	QwtPlotPanner* panner = new QwtPlotPanner( ui->plot->canvas() );
	panner->setAxisEnabled( QwtAxis::YRight, false );
	panner->setMouseButton( Qt::MiddleButton );

	// Avoid jumping when labels with more/less digits
	// appear/disappear when scrolling vertically

	const int extent = QwtPainter::horizontalAdvance(
		ui->plot->axisWidget( QwtAxis::YLeft )->fontMetrics(), "100.00" );

	ui->plot->axisScaleDraw( QwtAxis::YLeft )->setMinimumExtent( extent );

	const QColor c( Qt::darkBlue );
	zoomer->setRubberBandPen( c );
	zoomer->setTrackerPen( c );

	connect(ui->btnZoomAll, &QPushButton::clicked, this, &dlgQWTPlot::zoomAll);

}

dlgQWTPlot::~dlgQWTPlot()
{

	if (_plotdata!=nullptr)
	{
		if (_plotdata->get_plot_type()==PTQWT_DENSITY)
		{
			delete (QwtDensityPlot*)(ui->plot);
			ui->plot=nullptr;
		}
	}
	delete ui;
/*
	if (_plotdata!=nullptr)
		_plotdata.reset();

	_plotdata = nullptr;*/
}

void dlgQWTPlot::zoomAll()
{
	_plotdata->update_min_max(FULL);
	if ((_plotdata->get_plot_type()==PTQWT_PLOT)||(_plotdata->get_plot_type()==PTQWT_SCATTER))
	{
		double xmin = _plotdata->get_xmin();
		double xmax = _plotdata->get_xmax();
		double ymin = _plotdata->get_ymin();
		double ymax = _plotdata->get_ymax();
		ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
		ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
		ui->plot->replot();
	}
	if (_plotdata->get_plot_type()==PTQWT_DENSITY)
	{
		((QwtDensityPlot*)(ui->plot))->updateData((plotData_Density*)(_plotdata.get()));
		double xmin = _plotdata->get_xmin();
		double xmax = _plotdata->get_xmax();
		double ymin = _plotdata->get_ymin();
		double ymax = _plotdata->get_ymax();
		ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
		ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
		_density_plot_ref->updateZBar();
		_density_plot_ref->forceRedraw();
		ui->plot->replot();
		//((QwtDensityPlot*)(ui->plot))->setM
	}
}

void dlgQWTPlot::update_data(const std::set<std::string>& varlist)
{
	return;
	if (_plotdata==nullptr) return;
	if (_plotdata->update_data(varlist))
	{
		if ((_plotdata->get_plot_type()==PTQWT_PLOT)||(_plotdata->get_plot_type()==PTQWT_SCATTER))
		{
			QwtPlot_MinMaxUpdate zoomPolicy = QwtPlot_MinMaxUpdate (ui->cbZoom->currentIndex());
			_plotdata->update_min_max(zoomPolicy);
			double xmin = _plotdata->get_xmin();
			double xmax = _plotdata->get_xmax();
			double ymin = _plotdata->get_ymin();
			double ymax = _plotdata->get_ymax();

			ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
			ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
			ui->plot->replot();

		}
		if (_plotdata->get_plot_type()==PTQWT_DENSITY)
		{
			((QwtDensityPlot*)(ui->plot))->updateData((plotData_Density*)(_plotdata.get()));

			QwtPlot_MinMaxUpdate zoomPolicy = QwtPlot_MinMaxUpdate (ui->cbZoom->currentIndex());
			_plotdata->update_min_max(zoomPolicy);
			double xmin = _plotdata->get_xmin();
			double xmax = _plotdata->get_xmax();
			double ymin = _plotdata->get_ymin();
			double ymax = _plotdata->get_ymax();
			ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
			ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
			_density_plot_ref->updateZBar();
			_density_plot_ref->forceRedraw();
			ui->plot->replot();
		}
	}
}

void dlgQWTPlot::update_workspace()
{
	if (_plotdata==nullptr) return;
	if (_plotdata->update_data())
	{
		if ((_plotdata->get_plot_type()==PTQWT_PLOT)||(_plotdata->get_plot_type()==PTQWT_SCATTER))
		{
			QwtPlot_MinMaxUpdate zoomPolicy = QwtPlot_MinMaxUpdate (ui->cbZoom->currentIndex());
			_plotdata->update_min_max(zoomPolicy);
			double xmin = _plotdata->get_xmin();
			double xmax = _plotdata->get_xmax();
			double ymin = _plotdata->get_ymin();
			double ymax = _plotdata->get_ymax();
			ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
			ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
			ui->plot->replot();
		}
		if (_plotdata->get_plot_type()==PTQWT_DENSITY)
		{
			((QwtDensityPlot*)(ui->plot))->updateData((plotData_Density*)(_plotdata.get()));

			QwtPlot_MinMaxUpdate zoomPolicy = QwtPlot_MinMaxUpdate (ui->cbZoom->currentIndex());
			_plotdata->update_min_max(zoomPolicy);
			double xmin = _plotdata->get_xmin();
			double xmax = _plotdata->get_xmax();
			double ymin = _plotdata->get_ymin();
			double ymax = _plotdata->get_ymax();
			ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
			ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
			_density_plot_ref->updateZBar();
			_density_plot_ref->forceRedraw();
			ui->plot->replot();
		}
	}

}

void dlgQWTPlot::assign_vars(QStringList xvars, QStringList yvars)
{
	if (_plotdata==nullptr) return;

	if (_plotdata->get_plot_type()==PTQWT_DENSITY) return;

	_plotdata->set_data_plot(yvars,xvars);

}

void dlgQWTPlot::assign_vars(QString yname, QString xname)
{
	if (_plotdata==nullptr) return;

	if (_plotdata->get_plot_type()==PTQWT_DENSITY) return;

	_plotdata->set_data_plot(yname,xname);
	std::map<QString,PlotCurves> curves = _plotdata->get_curves();

	for (auto& curve_map: curves)
		for (auto curve : curve_map.second)
		{
			if (curve.first!=nullptr)
				curve.first->attach(ui->plot);
		}
	_plotdata->update_min_max(FULL);

	double xmin = _plotdata->get_xmin();
	double xmax = _plotdata->get_xmax();
	double ymin = _plotdata->get_ymin();
	double ymax = _plotdata->get_ymax();
	ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
	ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
	ui->plot->replot();
}


void dlgQWTPlot::assign_vars(QString zname, QString yname, QString xname)
{
	if (_plotdata==nullptr) return;

	if (_plotdata->get_plot_type()!=PTQWT_DENSITY)
		return;

	_plotdata->set_data_plot(zname,xname, yname);
	((QwtDensityPlot*)(ui->plot))->updateData((plotData_Density*)(_plotdata.get()));

	_plotdata->update_min_max(FULL);

	double xmin = _plotdata->get_xmin();
	double xmax = _plotdata->get_xmax();
	double ymin = _plotdata->get_ymin();
	double ymax = _plotdata->get_ymax();

	ui->plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
	ui->plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
}

bool dlgQWTPlot::has_var(QString vname)
{
	return _plotdata->has_var_in_list(vname);
}


void dlgQWTPlot::cbContourChanged(Qt::CheckState state)
{
	if (_plotdata->get_plot_type()!=PTQWT_DENSITY) return;
	if (_density_plot_ref==nullptr) return;
	_density_plot_ref->showContour(state = Qt::Checked);
}

void dlgQWTPlot::cbColorMapChanged(int index)
{
	if (_plotdata->get_plot_type()!=PTQWT_DENSITY) return;
	if (_density_plot_ref==nullptr) return;
	switch (index)
	{
	case 1:
		_density_plot_ref->setColorMap(QwtDensityPlot::HueMap);
		break;
	case 2:
		_density_plot_ref->setColorMap(QwtDensityPlot::SaturationMap);
		break;
	case 3:
		_density_plot_ref->setColorMap(QwtDensityPlot::ValueMap);
		break;
	case 4:
		_density_plot_ref->setColorMap(QwtDensityPlot::SVMap);
		break;
	case 5:
		_density_plot_ref->setColorMap(QwtDensityPlot::AlphaMap);
		break;
	default:
		_density_plot_ref->setColorMap(QwtDensityPlot::RGBMap);
		break;
	}
}

void QwtDensityPlot::updateContourValues()
{

	const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
	double dZ = zInterval.width()/10.0;
	QList< double > contourLevels;
	for ( double level = zInterval.minValue()+dZ/2; level < zInterval.maxValue(); level += dZ )
		contourLevels += level;

	m_spectrogram->setContourLevels( contourLevels );

	setAxisScale( QwtAxis::YRight, zInterval.minValue(), zInterval.maxValue() );
}

QwtDensityPlot::QwtDensityPlot(QWidget* parent,plotData_Density* data) : QwtPlot(parent),  m_alpha(255)
{
	m_spectrogram = new QwtImageSCPlot();
	m_spectrogram->setRenderThreadCount( 0 ); // use system specific thread count
	m_spectrogram->setCachePolicy( QwtPlotRasterItem::PaintCache );

	if (data==nullptr)
		m_spectrogram->setData( new plotData_Density() );
	else
		m_spectrogram->setData( data );

	m_spectrogram->attach( this );

	updateContourValues();

	// A color bar on the right axis
	QwtScaleWidget* rightAxis = axisWidget( QwtAxis::YRight );
	rightAxis->setTitle( "Intensity" );
	rightAxis->setColorBarEnabled( true );

	setAxisVisible( QwtAxis::YRight );

	plotLayout()->setAlignCanvasToScales( true );

	setColorMap( QwtDensityPlot::RGBMap );

}

void QwtDensityPlot::updateData(plotData_Density* data)
{
	if ((data==nullptr)||(m_spectrogram==nullptr)) return;
	m_spectrogram->setData(data);

	updateContourValues();
}

QwtDensityPlot::~QwtDensityPlot()
{
	m_spectrogram->detach();
//	if (m_spectrogram) delete m_spectrogram;
//	m_spectrogram = nullptr;
}


void QwtDensityPlot::showContour( bool on )
{
	m_spectrogram->setDisplayMode( QwtPlotSpectrogram::ContourMode, on );
	replot();
}

void QwtDensityPlot::showSpectrogram( bool on )
{
	m_spectrogram->setDisplayMode( QwtPlotSpectrogram::ImageMode, on );
	m_spectrogram->setDefaultContourPen(
		on ? QPen( Qt::black, 0 ) : QPen( Qt::NoPen ) );

	replot();
}

void QwtDensityPlot::setColorTableSize( int type )
{
	int numColors = 0;
	switch( type )
	{
	case 1:
		numColors = 256;
		break;
	case 2:
		numColors = 1024;
		break;
	case 3:
		numColors = 16384;
		break;
	}

	m_spectrogram->setColorTableSize( numColors );
	replot();
}

void QwtDensityPlot::updateZBar()
{
	//QwtScaleWidget* axis = axisWidget( QwtAxis::YRight );
	const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
	setAxisScale( QwtAxis::YRight, zInterval.minValue(), zInterval.maxValue() );
}

void QwtDensityPlot::setColorMap( int type )
{
	QwtScaleWidget* axis = axisWidget( QwtAxis::YRight );
	const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );

	m_mapType = type;

	const QwtColorMap::Format format = QwtColorMap::RGB;

	int alpha = m_alpha;
	switch( type )
	{
	case QwtDensityPlot::HueMap:
	{
		m_spectrogram->setColorMap( new HueColorMap( format ) );
		axis->setColorMap( zInterval, new HueColorMap( format ) );
		break;
	}
	case QwtDensityPlot::SaturationMap:
	{
		m_spectrogram->setColorMap( new SaturationColorMap( format ) );
		axis->setColorMap( zInterval, new SaturationColorMap( format ) );
		break;
	}
	case QwtDensityPlot::ValueMap:
	{
		m_spectrogram->setColorMap( new ValueColorMap( format ) );
		axis->setColorMap( zInterval, new ValueColorMap( format ) );
		break;
	}
	case QwtDensityPlot::SVMap:
	{
		m_spectrogram->setColorMap( new SVColorMap( format ) );
		axis->setColorMap( zInterval, new SVColorMap( format ) );
		break;
	}
	case QwtDensityPlot::AlphaMap:
	{
		alpha = 255;
		m_spectrogram->setColorMap( new AlphaColorMap( format ) );
		axis->setColorMap( zInterval, new AlphaColorMap( format ) );
		break;
	}
	case QwtDensityPlot::RGBMap:
	default:
	{
		m_spectrogram->setColorMap( new LinearColorMap( format ) );
		axis->setColorMap( zInterval, new LinearColorMap( format ) );
	}
	}
	m_spectrogram->setAlpha( alpha );

	replot();
}

void QwtDensityPlot::setAlpha( int alpha )
{
	// setting an alpha value doesn't make sense in combination
	// with a color map interpolating the alpha value

	m_alpha = alpha;
	if ( m_mapType != QwtDensityPlot::AlphaMap )
	{
		m_spectrogram->setAlpha( alpha );
		replot();
	}
}

void QwtDensityPlot::drawItems( QPainter* painter, const QRectF& canvasRect,
					 const QwtScaleMap maps[QwtAxis::AxisPositions] ) const
{
	QwtPlot::drawItems( painter, canvasRect, maps );
/*
	if ( m_spectrogram )
	{
		QwtImageSCPlot* spectrogram = static_cast< QwtImageSCPlot* >( m_spectrogram );

		QString info( "%1 x %2 pixels: %3 ms" );
		info = info.arg( spectrogram->renderedSize().width() );
		info = info.arg( spectrogram->renderedSize().height() );
		info = info.arg( spectrogram->elapsed() );

		QwtDensityPlot* plot = const_cast< QwtDensityPlot* >( this );
		plot->Q_EMIT rendered( info );
	}*/
}

#ifndef QT_NO_PRINTER

void QwtDensityPlot::printPlot()
{
	QPrinter printer( QPrinter::HighResolution );
#if QT_VERSION >= 0x050300
	printer.setPageOrientation( QPageLayout::Landscape );
#else
	printer.setOrientation( QPrinter::Landscape );
#endif
	printer.setOutputFileName( "spectrogram.pdf" );

	QPrintDialog dialog( &printer );
	if ( dialog.exec() )
	{
		QwtPlotRenderer renderer;

		if ( printer.colorMode() == QPrinter::GrayScale )
		{
			renderer.setDiscardFlag( QwtPlotRenderer::DiscardBackground );
			renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasBackground );
			renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasFrame );
			renderer.setLayoutFlag( QwtPlotRenderer::FrameWithScales );
		}

		renderer.renderTo( this, printer );
	}
}
#endif

