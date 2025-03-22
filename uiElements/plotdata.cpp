#include "plotdata.h"
#include <QColor>
#include <dlgqwtplot.h>

inline double SQR(double x) {return x*x;}


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


void plotData_plot::set_data_plot(QStringList var_name, QStringList x_name)
{
	if (var_name.count()!=x_name.count()) return;
	for (int n=0; n < var_name.count(); n++)
		set_data_plot(var_name[n],x_name[n]);
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


	if (policy == MAXHOLD_NODECAY)
	{
		if (ymax > _ymax)
			_ymax = ymax;


		if (ymin < _ymin)
			_ymin = ymin;
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
plotData_Density::plotData_Density(octavews* ws, QwtPlot* parent) : QwtRasterData(), plotData(ws,parent,PTQWT_DENSITY),
	xaxis({"",ArraySize({0,nullptr})}),
	yaxis({"",ArraySize({0,nullptr})}),
	zvalues("",ArraySize({0,nullptr})),
	_xmin(-1.5), _xmax(1.5), _ymin(-1.5), _ymax(1.5), _zmin(-1.0), _zmax(1.0)
{

	setAttribute( QwtRasterData::WithoutGaps, true );

	m_intervals[ Qt::XAxis ] = QwtInterval( _xmin, _xmax );
	m_intervals[ Qt::YAxis ] = QwtInterval( _ymin, _ymax );
	m_intervals[ Qt::ZAxis ] = QwtInterval( _zmin, _zmax );
}


plotData_Density::~plotData_Density()
{

}


void    plotData_Density::update_min_max(QwtPlot_MinMaxUpdate policy)
{
	if (policy==CUSTOM) return;

	size_t npt_x = xaxis.second.first;
	size_t npt_y = yaxis.second.first;

	double* zValPtr = zvalues.second.second;

	if (xaxis.second.second!=nullptr)
	{
		_xmin = *(xaxis.second.second);
		_xmax = *(xaxis.second.second + npt_x -1 );
	}
	else
	{
		_xmin = 0;
		_xmax = double(npt_x-1);
	}

	if (yaxis.second.second!=nullptr)
	{
		_ymin = *(yaxis.second.second);
		_ymax = *(yaxis.second.second + npt_y -1);
	}
	else
	{
		_ymin = 0;
		_ymax = double(npt_y-1);
	}

	double zmax = -9e99, zmin = 9e99;

	if (zValPtr==nullptr) return;

	for (size_t x = 0; x < npt_x; x++)
		for (size_t y = 0; y < npt_y; y++, zValPtr++)
		{
			double zVal = *zValPtr;
			if (zVal < zmin) zmin = zVal;
			if (zVal > zmax) zmax = zVal;
		}

	if (policy == MAXHOLD)
	{
		if (zmax > _zmax)
			_zmax = zmax;
		else
			_zmax = 0.999 * _zmax + 0.001 * zmax;


		if (zmin < _zmin)
			_zmin = zmin;
		else
			_zmin = 0.999 * _zmin + 0.001 * zmin;
	}
	else
	{
		if (policy == MAXHOLD_NODECAY)
		{
			if (zmax > _zmax)
				_zmax = zmax;

			if (zmin < _zmin)
				_zmin = zmin;
		}
		else
		{
			_zmin = zmin; _zmax = zmax;
		}

	}


	m_intervals[ Qt::XAxis ] = QwtInterval( _xmin, _xmax );
	m_intervals[ Qt::YAxis ] = QwtInterval( _ymin, _ymax );
	m_intervals[ Qt::ZAxis ] = QwtInterval( _zmin, _zmax );
}

QwtInterval plotData_Density::interval( Qt::Axis axis ) const
{
	if ( axis >= 0 && axis <= 2 )
		return m_intervals[ axis ];

	return QwtInterval();
}

double plotData_Density::value( double x, double y ) const
{
/*
	const double c = 0.842;
	//const double c = 0.33;

	const double v1 = x * x + ( y - c ) * ( y + c );
	const double v2 = x * ( y + c ) + x * ( y + c );

	return 1.0 / ( v1 * v1 + v2 * v2 );*/	
	if (zvalues.second.second==nullptr) return 0.0;

	size_t npt_x = xaxis.second.first;
	size_t npt_y = yaxis.second.first;
	double x0=0, x1=0;
	double y0=0, y1=0;
	size_t ix0=0, ix1=0, iy0=0, iy1=0;
	if (xaxis.second.second==nullptr)
	{
		if (x < 0)
		{
			ix0 = ix1 = (x0 = x1 = 0);
		}
		else
			if (x >= npt_x - 1)
			{
				ix1 = ix0 = (ix0 = ix1 = npt_x -1);
			}
			else
			{
				ix0 = (x0 = floor(x));
				ix1 = (x1 = ceil(x));
			}
	}
	else
	{
		double *xPtr = xaxis.second.second;
		double *xMax = xPtr + (npt_x-1);
		if (x < *xPtr)
		{
			ix0 = ix1 = 0;
			x0 = x1 = *xPtr;
		}
		else
			if (x > *xMax)
			{
				ix0 = ix1 = npt_x-1;
				x0 = x1 = *xMax;
			}
			else
			{
				for (size_t n=0; n < npt_x-1; n++)
				{
					if ((x >= *xPtr)&&( x < *(++xPtr)))
					{
						ix0 = n;
						ix1 = n+1;
						x1  = *xPtr;
						x0 = *(xPtr-1);
						break;
					}
				}
			}
	}

	if (yaxis.second.second==nullptr)
	{
		if (y < 0)
		{
			iy0 = iy1 = (y0 = y1 = 0);
		}
		else
			if (y >= npt_y - 1)
			{
				iy1 = iy0 = (iy0 = iy1 = npt_y -1);
			}
			else
			{
				iy0 = (y0 = floor(x));
				iy1 = (y1 = ceil(x));
			}
	}
	else
	{
		double *yPtr = yaxis.second.second;
		double *yMax = yPtr + (npt_y-1);
		if (y < *yPtr)
		{
			iy0 = iy1 = 0;
			y0 = y1 = *yPtr;
		}
		else
			if (y > *yMax)
			{
				iy0 = iy1 = npt_y-1;
				y0  = y1 = *yMax;
			}
			else
			{
				for (size_t n=0; n < npt_y-1; n++)
				{
					if ((y >= *yPtr)&&( y < *(++yPtr)))
					{
						iy0 = n;
						iy1 = n+1;
						y1  = *yPtr;
						y0 = *(yPtr-1);
						break;
					}
				}
			}
	}

	size_t i00 = ix0 * (npt_y) + iy0;
	size_t i10 = ix1 * (npt_y) + iy0;
	size_t i01 = ix0 * (npt_y) + iy1;
	size_t i11 = ix1 * (npt_y) + iy1;

	double* zv = zvalues.second.second;
	double z00 = zv[i00];
	double z10 = zv[i10];
	double z01 = zv[i01];
	double z11 = zv[i11];

	double d0 = SQR(x-x0)+SQR(y-y0);
	double d1 = SQR(x-x1)+SQR(y-y1);
	// Let's assume a dual tet surface for linear interpolation

	double val;
	if (d0 < d1)
	{
		// p(x,y) = p(x0,y0) + dp/dx * (x-x0) + dp/dy * (y-y0)
		val = z00;
		if (ix0!=ix1)
		{
			double dx = x1-x0;
			double dp_dx = (z10-z00)/(dx);
			val += dp_dx * (x-x0);
		}
		if(iy0!=iy1)
		{
			double dy = y1-y0;
			double dp_dy = (z01-z00)/(dy);
			val += dp_dy * (y-y0);
		}
	}
	else
	{
		// p(x,y) = p(x1,y1) + dp/dx * (x-x1) + dp/dy * (y-y1)
		val = z11;
		if (ix0!=ix1)
		{
			double dx = x0-x1;
			double dp_dx = (z01-z11)/(dx);
			val += dp_dx * (x-x1);
		}
		if(iy0!=iy1)
		{
			double dy = y0-y1;
			double dp_dy = (z10-z11)/(dy);
			val += dp_dy * (y-y1);
		}

	}
	return val;

}

void	plotData_Density::fill_data_x(const NDArray &xdata)
{
	size_t npt = xdata.numel();
	if (xaxis.second.first!=npt)
	{
		if (xaxis.second.second!=nullptr)
			delete xaxis.second.second;
		xaxis.second = ArraySize(npt, new double[npt]);
	}
	memcpy(xaxis.second.second, xdata.data(), npt*sizeof(double));
}
void	plotData_Density::fill_data_y(const NDArray &ydata)
{
	size_t npt = ydata.numel();
	if (yaxis.second.first!=npt)
	{
		if (yaxis.second.second!=nullptr)
			delete yaxis.second.second;
		yaxis.second = ArraySize(npt, new double[npt]);
	}
	memcpy(yaxis.second.second, ydata.data(), npt*sizeof(double));
}


void	plotData_Density::fill_data_z(const NDArray &zdata)
{
	size_t npt = zdata.numel();
	if (zvalues.second.first!=npt)
	{
		if (zvalues.second.second!=nullptr)
			delete zvalues.second.second;
		zvalues.second = ArraySize(npt, new double[npt]);
	}
	memcpy(zvalues.second.second, zdata.data(), npt*sizeof(double));
}



void plotData_Density::set_data_plot(QString var_name, QString x_name, QString y_name)
{
	clean_data();

	if (workspace()==nullptr) return;

	octave_value zov = workspace()->var_value(var_name.toStdString());

	if (!x_name.isEmpty())
	{
		octave_value xov = workspace()->var_value(x_name.toStdString());
		if (xov.iscomplex())
			return;

		xaxis.first = x_name;
		fill_data_x(xov.array_value());		
	}
	else
	{
		size_t nptx = zov.dims()(1);
		NDArray xs(dim_vector({1,(octave_idx_type)nptx}));
		for (size_t ix=0; ix < nptx; ix++)
			xs(ix)=(double)(ix);
		xaxis.first="";
		fill_data_x(xs);
	}

	if (!y_name.isEmpty())
	{
		octave_value yov = workspace()->var_value(y_name.toStdString());
		if (yov.iscomplex())
			return;

		yaxis.first = y_name;
		fill_data_y(yov.array_value());
	}
	else
	{
		size_t npty = zov.dims()(0);
		NDArray ys(dim_vector({1,(octave_idx_type)npty}));
		for (size_t iy=0; iy < npty; iy++)
			ys(iy)=(double)(iy);
		yaxis.first="";
		fill_data_y(ys);
	}

	zvalues.first = var_name;
	if (zov.iscomplex())
	{
		fill_data_z(zov.complex_array_value().abs());
	}
	else
	{
		fill_data_z(zov.array_value());
	}
}

bool plotData_Density::update_data(const std::set<std::string>& varlist)
{
	bool bModified = false;
	for (auto& var: varlist)
	{
		if (var == xaxis.first.toStdString())
		{
			octave_value xov = workspace()->var_value(xaxis.first.toStdString());
			if (xov.iscomplex())
				return false;
			fill_data_x(xov.array_value());

			bModified = true;
		}

		if (var == yaxis.first.toStdString())
		{
			octave_value yov = workspace()->var_value(yaxis.first.toStdString());
			if (yov.iscomplex())
				return false;
			fill_data_y(yov.array_value());

			bModified = true;
		}

		if (var == zvalues.first.toStdString())
		{
			octave_value zov = workspace()->var_value(zvalues.first.toStdString());
			if (zov.iscomplex())
				return false;
			fill_data_y(zov.iscomplex() ? zov.complex_array_value().abs() : zov.array_value());

			if (xaxis.first.isEmpty())
			{
				size_t currentSizeX = xaxis.second.first;
				size_t requiredSizeX= zov.dims()(1);
				if (currentSizeX!=requiredSizeX)
				{
					if (xaxis.second.second!=nullptr)
					{
						delete xaxis.second.second;
						xaxis.second = ArraySize(requiredSizeX, new double[requiredSizeX]);
					}
					for (size_t x=0; x < requiredSizeX; x++)
						xaxis.second.second[x]=double(x);
				}
			}

			if (yaxis.first.isEmpty())
			{
				size_t currentSizeY = yaxis.second.first;
				size_t requiredSizeY= zov.dims()(0);
				if (currentSizeY!=requiredSizeY)
				{
					if (yaxis.second.second!=nullptr)
					{
						delete yaxis.second.second;
						yaxis.second = ArraySize(requiredSizeY, new double[requiredSizeY]);
					}
					for (size_t y=0; y < requiredSizeY; y++)
						yaxis.second.second[y]=double(y);
				}
			}

			bModified = true;
		}
	}
	return bModified;
}

bool plotData_Density::update_data()
{
	octave_value zov = workspace()->var_value(zvalues.first.toStdString());
	if (xaxis.first.isEmpty())
	{
		size_t currentSizeX = xaxis.second.first;
		size_t requiredSizeX= zov.dims()(1);
		if (currentSizeX!=requiredSizeX)
		{
			if (xaxis.second.second!=nullptr)
			{
				delete xaxis.second.second;
				xaxis.second = ArraySize(requiredSizeX, new double[requiredSizeX]);
			}
			for (size_t x=0; x < requiredSizeX; x++)
				xaxis.second.second[x]=double(x);
		}
	}
	else
	{
		octave_value xov = workspace()->var_value(xaxis.first.toStdString());
		if (xov.iscomplex())
			return false;
		fill_data_x(xov.array_value());
	}

	if (yaxis.first.isEmpty())
	{
		size_t currentSizeY = yaxis.second.first;
		size_t requiredSizeY= zov.dims()(0);
		if (currentSizeY!=requiredSizeY)
		{
			if (yaxis.second.second!=nullptr)
			{
				delete yaxis.second.second;
				yaxis.second = ArraySize(requiredSizeY, new double[requiredSizeY]);
			}
			for (size_t y=0; y < requiredSizeY; y++)
				yaxis.second.second[y]=double(y);
		}
	}
	else
	{
		octave_value yov = workspace()->var_value(yaxis.first.toStdString());
		if (yov.iscomplex())
			return false;

		fill_data_y(yov.array_value());
	}

	if (zov.iscomplex())
		fill_data_z(zov.complex_array_value().abs());
	else
		fill_data_z(zov.array_value());

	return true;
}

void plotData_Density::clean_data()
{
	if (xaxis.second.second!=nullptr)
		delete xaxis.second.second;

	xaxis.first="";
	xaxis.second.second = nullptr;
	xaxis.second.first = 0;

	if (yaxis.second.second!=nullptr)
		delete yaxis.second.second;

	yaxis.first="";
	yaxis.second.second = nullptr;
	yaxis.second.first = 0;
}


bool plotData_Density::has_var_in_list(const QStringList& var_changed)
{
	return false;
}

bool plotData_Density::has_var_in_list(const QString& var_changed)
{
	return false;
}
