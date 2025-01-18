#include "plotdata.h"
#include <QColor>
#include <dlgqwtplot.h>



plotData_plot::~plotData_plot()
{
	clean_data();
}

void plotData_plot::clean_data()
{
	for (auto& curve_var: curves)
	{
		for (auto curve: curve_var.second)
		{
			if (curve.first !=nullptr)
			{
				curve.first->detach();
				delete curve.first;
			}
			if (curve.second.second!=nullptr)
				delete curve.second.second;
		}

		curve_var.second.clear();
	}
	curves.clear();

	for (auto& xdata: xvals)
	{
		if (xdata.second.second!=nullptr)
			delete xdata.second.second;
	}
	xvals.clear();
	x_y.clear();
}



// This function is to *create* a new datastructure: single variable that can be either a vector or a matrix
// If it is a vector, and x_name is given, it must contain the same number of points.
// If it is a matrix, and x_name is given, the number of columns must match the number of x points
//				      if not, each row is a curve
void plotData_plot::set_data_plot(QString var_name, QString x_name)
{
	clean_data();
	if (workspace()==nullptr) return;
	octave_value yov = workspace()->var_value(var_name.toStdString());

	if (yov.ndims()!=2) return;

	size_t xlength=0, ncurves = 0;
	octave_value x;

	if (!x_name.isEmpty())
	{
		x = workspace()->var_value(x_name.toStdString());
		xlength = x.numel();

		size_t rows = yov.dims()(0);
		size_t cols = yov.dims()(1);

		if (cols!=xlength)
			return;

		ncurves = rows;

		x_y.insert_or_assign(var_name,x_name);
	}
	else
	{
		// Pick the lowest number of curves
		size_t rows = yov.dims()(0);
		size_t cols = yov.dims()(1);

		ncurves = rows < cols ? rows : cols;
		xlength = cols < rows ? rows : cols;

		x_y.erase(var_name);
	}

	manage_data_curve(var_name, x_name, ncurves, xlength);
	if (yov.iscomplex())
		fill_data_y(var_name,yov.complex_array_value().abs());
	else
		fill_data_y(var_name,yov.array_value());
	if (!x_name.isEmpty())
		fill_data_x(x_name,x.array_value());

}


void plotData_plot::set_data_plot(QStringList var_name, QString x_name)
{

}

/*
 * Create data curve and associated storage. Return true if any changes has been done
*/
bool plotData_plot::manage_data_curve  (const QString& yname, const QString& xname, size_t ncurves, size_t npt)
{
	if (yname.isEmpty()) return false;
	auto  found = curves.insert({yname, PlotCurves()});

	bool bNew = found.second;
	bool bOut = false;
	auto&  qwtCurves = found.first->second;
	double *xMemData = nullptr;
	if (bNew)
	{
		style = Qt::SolidLine;
		// Increment color
		if (gc != Qt::darkYellow)
			gc = (Qt::GlobalColor)(gc+1);
		else
			gc = Qt::red;
	}

	if (!xname.isEmpty())
	{
		// Delete data if present
		std::pair<std::map<QString, ArraySize>::iterator, bool> xData = xvals.insert({xname,ArraySize(0,nullptr)});

		if (xData.first->second.first!=npt)
		{
			xData.first->second.first = npt;
			if (xData.first->second.second!=nullptr)
				delete xData.first->second.second;
			xMemData = new double[npt];
			xData.first->second.second = xMemData;
			xData.first->second.first = npt;
		}
		bOut = true;
	}


	if (qwtCurves.count()!=ncurves)
	{
		QColor color(gc);


		// Create new curves if required
		if (ncurves > qwtCurves.count())
		{
			for (size_t c = qwtCurves.count(); c < ncurves; c++)
			{
				//if (objSymbol==nullptr)
					objSymbol = new QwtSymbol(symbol,QBrush(gc), QPen(gc,2), QSize(5,5));

				QwtPlotCurve* newCurve = new QwtPlotCurve("");
				newCurve->setPen(color,2,style);

				newCurve->setSymbol(objSymbol);
				newCurve->setRenderHint( QwtPlotItem::RenderAntialiased, true );

				// Create new data
				double* yMemData = new double[npt];
				qwtCurves.append( Curve(newCurve, ArraySize(npt,yMemData)));
				// Assign
				if (xMemData==nullptr)
					newCurve->setRawSamples(yMemData,npt);
				else
					newCurve->setRawSamples(xMemData,yMemData, npt);

				if (parent()!=nullptr)
					newCurve->attach(parent());

				if (style != Qt::DashDotDotLine)
					style = (Qt::PenStyle)(style+1);
				else
				{
					style = Qt::SolidLine;
					if (symbol == QwtSymbol::Hexagon)
						symbol  = QwtSymbol::Ellipse;
					else
						symbol = QwtSymbol::Style(symbol+1);

					objSymbol = new QwtSymbol(symbol,QBrush(gc), QPen(gc,2), QSize(5,5));
				}
			}
			bOut = true;
		}

		if (qwtCurves.count() > ncurves)
		{
			for (size_t c = ncurves; c < qwtCurves.count(); c++)
			{
				if (qwtCurves[c].first!=nullptr)
				{
					qwtCurves[c].first->detach();
					delete qwtCurves[c].first;
				}

				if (qwtCurves[c].second.second!=nullptr)
					delete qwtCurves[c].second.second;
			}

			qwtCurves.remove(ncurves, qwtCurves.count() - ncurves);

			bOut = true;
		}

		// Update names
		if (bOut)
			for (size_t c = 0; c < ncurves; c++)
				qwtCurves[c].first->setTitle(yname+"_"+QString::number(c));
	}

	// Check if array length is ok
	for (auto& curve: qwtCurves)
	{
		if (curve.second.first!=npt)
		{
			curve.second.first = npt;
			if (curve.second.second != nullptr) delete curve.second.second;

			double* yMemData = new double[npt];
			curve.second.second = yMemData;
			curve.second.first = npt;

			if (xMemData==nullptr)
				curve.first->setRawSamples(yMemData,npt);
			else
				curve.first->setRawSamples(xMemData,yMemData, npt);
			bOut = true;
		}
	}

	return bOut;
}

bool	plotData_plot::fill_data_y(QString vname, const NDArray& data)
{
	// Assume that all arrays have been properly set
	auto found = curves.find(vname);
	if (found==curves.end()) return false;
	if (found->second.count()!=data.dim1()) return false;
	for (size_t row = 0; row < data.dim1(); row++)
	{
		ArraySize array = found->second[row].second;
		if (array.first!=data.dim2())
			continue;
		if (data.dim1()==1)
			memcpy(array.second, data.data(), sizeof(double)*array.first);
		else
		{
			for (int col = 0; col < data.dim2(); col++)
			{
				array.second[col] = data(row,col);
				//qDebug() << array.second[col];
			}
		}
	}

	return true;
}

bool	plotData_plot::fill_data_x(QString vname, const NDArray& data)
{
	auto found = xvals.find(vname);
	if (found==xvals.end()) return false;
	if (found->second.first != data.dim1() * data.dim2()) return false;
	memcpy(found->second.second, data.data(), found->second.first * sizeof(double));
	return true;
}


bool	plotData_plot::update_data(const std::set<std::string>& varlist)
{
	if (workspace()==nullptr) return false;

	QString xname="";
	bool bAnyVarModified = false;
	for (auto& var: varlist)
	{
		QString varname = QString::fromStdString(var);
		auto curve = curves.find(varname);
		// Check if it is an y value
		if (curve!=curves.end())
		{
			octave_value data = workspace()->var_value(var);

			NDArray y = data.iscomplex() ? data.complex_array_value().abs() : workspace()->var_value(var).array_value();
			// Check if we already have the x
			std::map<QString,QString>::iterator xy_map = x_y.find(varname);
			if (xy_map!=x_y.end())
				xname = xy_map->second;

			manage_data_curve(varname, xname, y.dim1(), y.dim2());
			fill_data_y(varname, y);

			bAnyVarModified = true;
		}
		auto xarray = xvals.find(varname);
		if (xarray!=xvals.end())
		{
			NDArray x = workspace()->var_value(var).array_value();

			fill_data_x(varname, x);

			bAnyVarModified = true;
		}
	}
	return bAnyVarModified;
}

bool	plotData_plot::update_data()
{
	if (workspace()==nullptr) return false;

	for (auto& curve: curves)
	{
		QString yname = curve.first;
		std::map<QString,QString>::iterator xy_found = x_y.find(yname);
		QString xname = "";

		if (xy_found != x_y.end())
			xname = xy_found->second;

		octave_value data = workspace()->var_value(yname.toStdString());

		NDArray y = data.iscomplex() ? data.complex_array_value().abs() : workspace()->var_value(yname.toStdString()).array_value();

		manage_data_curve(yname, xname, y.dim1(), y.dim2());
		fill_data_y(yname, y);

		if (xy_found!=x_y.end())
		{
			NDArray x;
			x = workspace()->var_value(xname.toStdString()).array_value();
			fill_data_x(xname, x);
		}
	}

	return true;
}


bool	plotData_plot::has_var_in_list(const QStringList& var_changed)
{
	for (const auto& vname : var_changed)
	{
		if (xvals.find(vname)!=xvals.end())
			return true;
		if (curves.find(vname)!=curves.end())
			return true;
	}
	return false;
}

bool	plotData_plot::has_var_in_list(const QString& var_changed)
{
	if (xvals.find(var_changed)!=xvals.end())
		return true;
	if (curves.find(var_changed)!=curves.end())
		return true;

	return false;
}



void    plotData_plot::update_min_max(QwtPlot_MinMaxUpdate policy)
{

	if (policy==CUSTOM) return;

	double ymax = -9e99;
	double ymin = 9e99;
	// Calculate curves max values

	double xmax = -9e99;
	double xmin = 9e99;

	for (auto& curve: curves)
	{
		double npt_max = 0;
		for (auto& plot_value : curve.second)
		{
			ArraySize& array = plot_value.second;
			if (array.second==nullptr) continue;
			double* data_ptr = array.second;
			for (size_t n=0; n < array.first; n++, data_ptr++)
			{
				if ((*data_ptr) > ymax) ymax = (*data_ptr);
				if ((*data_ptr) < ymin) ymin = (*data_ptr);
			}
			if (array.first > npt_max)
				npt_max = (double)(array.first);
		}

		QString yname = curve.first;
		std::map<QString, QString>::iterator	xmap = x_y.find(yname);
		std::map<QString, ArraySize>::iterator	xv   = xmap == x_y.end() ? xvals.end() : xvals.find(xmap->second);

		if (xv != xvals.end())
		{
			double* data_ptr = xv->second.second;
			for (size_t n=0; n < xv->second.first; n++, data_ptr++)
			{
				if ((*data_ptr) > xmax) xmax = (*data_ptr);
				if ((*data_ptr) < xmin) xmin = (*data_ptr);
			}
		}
		else
		{
			if (xmin > 0) xmin = 0;
			if (npt_max-1 > xmax) xmax = npt_max-1;
		}
	}

	if (policy == MAXHOLD)
	{
		if (ymax > _ymax)
			_ymax = ymax;
		else
			_ymax = 0.999 * _ymax + 0.001 * ymax;


		if (ymin < _ymin)
			_ymin = ymin;
		else
			_ymin = 0.999 * _ymin + 0.001 * ymin;
	}
	if (policy==FULL)
	{
		_ymax = ymax;
		_ymin = ymin;
	}

	_xmax = xmax;
	_xmin = xmin;
}



//--------------------------------------------------------------------------
// Density raster
plotData_Density::plotData_Density(octavews* ws, QwtPlot* parent) : QwtRasterData(), plotData(ws,parent,PTQWT_DENSITY)
{

	setAttribute( QwtRasterData::WithoutGaps, true );

	m_intervals[ Qt::XAxis ] = QwtInterval( -1.5, 1.5 );
	m_intervals[ Qt::YAxis ] = QwtInterval( -1.5, 1.5 );
	m_intervals[ Qt::ZAxis ] = QwtInterval( 0.0, 10.0 );
}


plotData_Density::~plotData_Density()
{

}


void    plotData_Density::update_min_max(QwtPlot_MinMaxUpdate policy)
{

}

QwtInterval plotData_Density::interval( Qt::Axis axis ) const
{
	if ( axis >= 0 && axis <= 2 )
		return m_intervals[ axis ];

	return QwtInterval();
}

double plotData_Density::value( double x, double y ) const
{
	const double c = 0.842;
	//const double c = 0.33;

	const double v1 = x * x + ( y - c ) * ( y + c );
	const double v2 = x * ( y + c ) + x * ( y + c );

	return 1.0 / ( v1 * v1 + v2 * v2 );
}



void plotData_Density::set_data_plot(QString var_name, QString x_name, QString y_name)
{

}

bool plotData_Density::update_data(const std::set<std::string>& varlist)
{
	return false;
}

bool plotData_Density::update_data()
{
	return false;
}
void plotData_Density::clean_data()
{

}
bool plotData_Density::has_var_in_list(const QStringList& var_changed)
{
	return false;
}

bool plotData_Density::has_var_in_list(const QString& var_changed)
{
	return false;
}
