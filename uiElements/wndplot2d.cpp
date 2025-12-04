/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wndplot2d.h"
#include "graphs/jkqtplines.h"
#include "mdioctaveinterface.h"
#include "ui_wndplot2d.h"
#include "jkqtplotter/jkqtplotter.h"
#include "jkqtplotter/graphs/jkqtpfilledcurve.h"
#include "jkqtplotter/graphs/jkqtpboxplot.h"
#include "jkqtplotter/graphs/jkqtpbarchart.h"
#include "jkqtplotter/graphs/jkqtpboxplot.h"
#include "jkqtplotter/graphs/jkqtpscatter.h"
#include "jkqtplotter/graphs/jkqtpimage.h"
#include "jkqtplotter/graphs/jkqtpvectorfield.h"
#include "jkqtplotter/graphs/jkqtpimpulses.h"
#include "../mainwindow.h"

#include <ariautils.h>

double Lerp(double v0, double v1, double t)
{
    return (1 - t)*v0 + t*v1;
};


NDArray Quantile(const octave_value& inData)
{

    NDArray quantiles;
    quantiles.resize(dim_vector{1,3});
    if (inData.numel()==0)
    {
        return NDArray();
    }

    if (1 == inData.numel())
        return inData.array_value();

    NDArray data(inData.sort(1).array_value());
/*
    for (int n=0; n < data.numel(); n++)
        qDebug() << data(n);
*/
    std::vector<double> probs={0.25, 0.5, 0.75};

    for (size_t i = 0; i < probs.size(); ++i)
    {
        double poi = Lerp(-0.5, data.numel() - 0.5, probs[i]);
//        qDebug() << probs[i];
//        qDebug() << data.numel();
//        qDebug() << poi;

        size_t left = std::max(int64_t(std::floor(poi)), int64_t(0));
        size_t right = std::min(int64_t(std::ceil(poi)), int64_t(data.numel() - 1));

        double datLeft = data(left);
        double datRight = data(right);

//        qDebug() << left;
//        qDebug() << right;
//        qDebug() << datLeft;
//        qDebug() << datRight;

        double quantile = Lerp(datLeft, datRight, poi - left);

        quantiles(i) = quantile;
    }

    return quantiles;
}


wndPlot2d::wndPlot2d(mdiOctaveInterface* parent, octavews* ws) :
    QDialog(parent),
    ui(new Ui::wndPlot2d),
    _workspace(ws),
    _nplot_x(0), _nplot_y(0), _total_plot(0),
    _plots(),
    _plotters(),
    _last_error(""),
    _owner_mdiOctave(parent),
    _owner_mainWnd(nullptr)

{
    ui->setupUi(this);

    _layout = new QGridLayout();
    ui->mainWidget->setLayout(_layout);
}


wndPlot2d::wndPlot2d(MainWindow* parent, octavews* ws) :
    QDialog(parent),
    ui(new Ui::wndPlot2d),
    _workspace(ws),
    _nplot_x(0), _nplot_y(0), _total_plot(0),
    _plots(),
    _plotters(),
    _last_error(""),
    _owner_mdiOctave(nullptr),
    _owner_mainWnd(parent)

{
    ui->setupUi(this);

    _layout = new QGridLayout();
    ui->mainWidget->setLayout(_layout);
}

wndPlot2d::~wndPlot2d()
{
    if (_owner_mdiOctave!=nullptr)
        _owner_mdiOctave->delete_children(this);
    if (_owner_mainWnd!=nullptr)
        _owner_mainWnd->delete_children(this);

    remove_workspace();
    delete ui;
    delete _layout;
    clear_plots();

}

void wndPlot2d::clear_owners()
{
    _owner_mainWnd = nullptr;
    _owner_mdiOctave=nullptr;
}

JKQTPDatastore* wndPlot2d::get_ds()
{
    if (_plotters.empty()) return nullptr;
    if (_plotters[0]==nullptr) return nullptr;
    return (_plotters[0]->getDatastore());
}
//---------------------------------------------------------------------
// Populate plot: here we already have data inside the datastore:
// In populate_xxxx the pre-created graphs (e.g. a plot curve, a scatter serie etc.)
// are linked to the data into the datastore
//---------------------------------------------------------------------
void wndPlot2d::populate_plot( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_PLOT) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    for (size_t g=0; g < pl._graph.size(); g++)
    {
       // Data is already in the datastore, we just need to add the proper X-Y data to the graph
        JKQTPXYLineGraph* line_graph = (JKQTPXYLineGraph*) pl._graph[g];
        if (line_graph==nullptr)
            continue;


        if (!pl._indep_x.empty())
        {
            if (line_graph->getXColumn()==-1)
                line_graph->setXColumn(ds->getColumnNum(QString::fromStdString(pl._indep_x)));
        }
        else
            if (line_graph->getXColumn()==-1)
                line_graph->setXColumn(ds->getColumnNum(get_var_indep_name(pl._indep_x, pl._dep)));

        if (!pl._dep.empty())
        {
            int nrows = get_var_count(pl._dep);
            if (nrows==1)
            {
                QString vname = QString::fromStdString(pl._dep);
				if (line_graph->getYColumn()==-1)
				{
					line_graph->setYColumn(ds->getColumnNum(vname));
					line_graph->setTitle(QString::fromStdString(pl._dep));
				}
            }
            else
            {
                QString name = get_var_name(pl._dep, g);

                if (line_graph->getYColumn()==-1)
				{
                    line_graph->setYColumn(ds->getColumnNum(name));
					line_graph->setTitle(name);
				}
            }
        }

    }
    redraw(pl);
}
//---------------------------------------------------------------------
// Populate box plot
//---------------------------------------------------------------------
void        wndPlot2d::populate_boxplot( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_BOXPLOT) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    if (pl._graph.size()!=2)
        return;
    // 1st graph is the proper boxplot
    JKQTPBoxplotVerticalGraph* boxplot_graph = (JKQTPBoxplotVerticalGraph* )pl._graph[0];
    JKQTPXYLineGraph* outlier_graph          = (JKQTPXYLineGraph*)pl._graph[1];

    if (boxplot_graph==nullptr) return;
    if (outlier_graph==nullptr) return;

    QString xname = get_var_indep_name(pl._indep_x, pl._dep);
    QString basename = QString::fromStdString(pl._dep);
    QString outlier_name = basename+"{outliers}";
    QString outlier_name_x = basename+"{outliers_x}";


    QString min_name = basename + "{min}";
    int var_id_min = ds->getColumnNum(min_name);

    //------------
    // max
    QString max_name = basename + "{max}";
    int var_id_max = ds->getColumnNum(max_name);
    //------------
    // q25
    QString q25_name = basename + "{Q25}";
    int var_id_q25 = ds->getColumnNum(q25_name);

    //------------
    // q75
    QString q75_name = basename + "{Q75}";
    int var_id_q75 = ds->getColumnNum(q75_name);
    //------------
    // q25
    QString mean_name = basename + "{Mean}";
    int var_id_mean = ds->getColumnNum(mean_name);
    //------------
    // q75
    QString median_name = basename + "{Median}";
    int var_id_median = ds->getColumnNum(median_name);

    if (boxplot_graph->getPositionColumn()==-1)
        boxplot_graph->setPositionColumn(ds->getColumnNum(xname));

    if (boxplot_graph->getMinColumn()==-1)
        boxplot_graph->setMinColumn(var_id_min);

    if (boxplot_graph->getPercentile25Column()==-1)
        boxplot_graph->setPercentile25Column(var_id_q25);

    if (boxplot_graph->getMedianColumn()==-1)
        boxplot_graph->setMedianColumn(var_id_median);

    if (boxplot_graph->getMeanColumn()==-1)
        boxplot_graph->setMeanColumn(var_id_mean);

    if (boxplot_graph->getPercentile75Column()==-1)
        boxplot_graph->setPercentile75Column(var_id_q75);

    if (boxplot_graph->getMaxColumn()==-1)
        boxplot_graph->setMaxColumn(var_id_max);

    if (outlier_graph->getXColumn()==-1)
        outlier_graph->setXColumn(ds->getColumnNum(outlier_name_x));

    if (outlier_graph->getYColumn()==-1)
        outlier_graph->setYColumn(ds->getColumnNum(outlier_name));

    plotter->zoomToFit();

    redraw(pl);

}
//---------------------------------------------------------------------
// Populate area plot
//---------------------------------------------------------------------

void        wndPlot2d::redraw(plot_descriptor& pl)
{
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (pl._init_zoom==false)
    {
        plotter->zoomToFit();
        pl._init_zoom = true;
    }
    else
		plotter->redrawPlot();

}

//---------------------------------------------------------------------
// Populate area plot
//---------------------------------------------------------------------

void        wndPlot2d::populate_area( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_AREA) return;
    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    for (size_t g=0; g < pl._graph.size(); g++)
    {
        // Data is already in the datastore, we just need to add the proper X-Y data to the graph
         JKQTPFilledCurveXGraph* filled_plot_graph = ( JKQTPFilledCurveXGraph*) pl._graph[g];
        if (filled_plot_graph==nullptr)
            continue;
        QColor col = filled_plot_graph->getFillColor();
        col.setAlphaF(0.25);
        filled_plot_graph->setFillColor(col);

        if (!pl._indep_x.empty())
        {
            //QString xaxis_label = plotter->getXAxis()->getAxisLabel();
            //xaxis_label += QString::fromStdString(pl._indep_x) + " ";
            //plotter->getXAxis()->setAxisLabel(xaxis_label);

            if (filled_plot_graph->getXColumn()==-1)
                filled_plot_graph->setXColumn(ds->getColumnNum(QString::fromStdString(pl._indep_x)));
        }
        else
        {
            if (filled_plot_graph->getXColumn()==-1)
                filled_plot_graph->setXColumn(ds->getColumnNum(get_var_indep_name(pl._indep_x, pl._dep)));
        }

        if (!pl._dep.empty())
        {
            int nrows = get_var_count(pl._dep);
            if (nrows==1)
            {
                QString vname = QString::fromStdString(pl._dep);

                if (filled_plot_graph->getYColumn()==-1)
                    filled_plot_graph->setYColumn(ds->getColumnNum(vname));

                if (filled_plot_graph->getTitle().isEmpty())
                    filled_plot_graph->setTitle(QString::fromStdString(pl._dep));
            }
            else
            {
                QString name = get_var_name(pl._dep, g);
                if (filled_plot_graph->getTitle().isEmpty())
                    filled_plot_graph->setTitle(name);

                if (filled_plot_graph->getYColumn()==-1)
                    filled_plot_graph->setYColumn(ds->getColumnNum(name));
            }
        }
    }

    redraw(pl);
}
//---------------------------------------------------------------------
// Populate vertical bars plot
//---------------------------------------------------------------------
void        wndPlot2d::populate_barv( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_BARV) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    for (size_t g=0; g < pl._graph.size(); g++)
    {
        // Data is already in the datastore, we just need to add the proper X-Y data to the graph
        JKQTPBarVerticalGraph* barv_graph = (JKQTPBarVerticalGraph*) pl._graph[g];
        if (barv_graph==nullptr)
            continue;


        if (!pl._indep_x.empty())
        {
            if (barv_graph->getXColumn()==-1)
                barv_graph->setXColumn(ds->getColumnNum(QString::fromStdString(pl._indep_x)));
            /*
            QString xaxis_label = plotter->getXAxis()->getAxisLabel();
            xaxis_label += QString::fromStdString(pl._indep_x) + " ";
            plotter->getXAxis()->setAxisLabel(xaxis_label);*/
        }
        else
            if (barv_graph->getXColumn()==-1)
                barv_graph->setXColumn(ds->getColumnNum(get_var_indep_name(pl._indep_x, pl._dep)));

        if (!pl._dep.empty())
        {
            int nrows = get_var_count(pl._dep);
            if (nrows==1)
            {
                QString vname = QString::fromStdString(pl._dep);
                if (barv_graph->getYColumn()==-1)
                    barv_graph->setYColumn(ds->getColumnNum(vname));

                if (barv_graph->getTitle().isEmpty())
                    barv_graph->setTitle(QString::fromStdString(pl._dep));
            }
            else
            {
                QString name = get_var_name(pl._dep, g);
                if (barv_graph->getTitle().isEmpty())
                    barv_graph->setTitle(name);
                if (barv_graph->getYColumn()==-1)
                    barv_graph->setYColumn(ds->getColumnNum(name));
            }
        }
    }

    redraw(pl);


}
//---------------------------------------------------------------------
// Populate scatter plot
//---------------------------------------------------------------------
void        wndPlot2d::populate_scatter( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_SCATTER) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    for (size_t g=0; g < pl._graph.size(); g++)
    {
        // Data is already in the datastore, we just need to add the proper X-Y data to the graph
        JKQTPXYScatterGraph* scatter_graph = (JKQTPXYScatterGraph*) pl._graph[g];
        if (scatter_graph==nullptr)
            continue;


        if (!pl._indep_x.empty())
        {
            if (scatter_graph->getXColumn()==-1)
                scatter_graph->setXColumn(ds->getColumnNum(QString::fromStdString(pl._indep_x)));
            /*
            QString xaxis_label = plotter->getXAxis()->getAxisLabel();
            xaxis_label += QString::fromStdString(pl._indep_x) + " ";
            plotter->getXAxis()->setAxisLabel(xaxis_label);*/
        }
        else
            if (scatter_graph->getXColumn()==-1)
                scatter_graph->setXColumn(ds->getColumnNum(get_var_indep_name(pl._indep_x, pl._dep)));

        if (!pl._dep.empty())
        {
            int nrows = get_var_count(pl._dep);
            if (nrows==1)
            {
                QString vname = QString::fromStdString(pl._dep);
                if (scatter_graph->getYColumn()==-1)
                    scatter_graph->setYColumn(ds->getColumnNum(vname));

                if (scatter_graph->getTitle().isEmpty())
                    scatter_graph->setTitle(QString::fromStdString(pl._dep));
            }
            else
            {
                QString name = get_var_name(pl._dep, g);
                if (scatter_graph->getTitle().isEmpty())
                    scatter_graph->setTitle(name);
                if (scatter_graph->getYColumn()==-1)
                    scatter_graph->setYColumn(ds->getColumnNum(name));
            }
        }
    }

    redraw(pl);
}
//---------------------------------------------------------------------
// Populate density (heatmap) plot
//---------------------------------------------------------------------

void        wndPlot2d::populate_dens( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_DENS) return;

     JKQTPDatastore* ds=get_ds();

    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    if (pl._graph.size()!=1)
        return;

    JKQTPColumnMathImage* density_graph = (JKQTPColumnMathImage*)pl._graph[0];

    if (density_graph == nullptr) return;

    QString xname = get_var_indep_name(pl._indep_x,pl._dep,0);
    QString yname = get_var_indep_name(pl._indep_y,pl._dep,1);
    int var_x  = ds->getColumnNum(xname);
    if (var_x==-1) return;
    int var_y  = ds->getColumnNum(yname);
    if (var_y==-1) return;

    int Nx = (int)(ds->get(var_x,2));
    int Ny = (int)(ds->get(var_y,2));
    double xmin = ds->get(var_x,0);
    double xmax = ds->get(var_x,1);
    double ymin = ds->get(var_y,0);
    double ymax = ds->get(var_y,1);


	if (density_graph->getImageColumn()==-1)
        density_graph->setImageColumn(ds->getColumnNum(QString::fromStdString(pl._dep)));

	redraw(pl);
	return;

    density_graph->setNx(Nx);
    density_graph->setNy(Ny);
    density_graph->setX(xmin);
    density_graph->setY(ymin);
    density_graph->setWidth(xmax-xmin);
    density_graph->setHeight(ymax-ymin);

    plotter->getPlotter()->setMaintainAspectRatio(true);
    plotter->getPlotter()->setMaintainAxisAspectRatio(true);
    density_graph->setTitle(QString::fromStdString(pl._dep));
    density_graph->getColorBarRightAxis()->setAxisLabel(QString::fromStdString(pl._dep)+ " intensity");
	//plotter->addGraph(density_graph);




}
//---------------------------------------------------------------------
// Populate contour (heatmap) plot
//---------------------------------------------------------------------

void        wndPlot2d::populate_contour( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_VECT2D) return;
}
//---------------------------------------------------------------------
// Populate 2D Vector plot
//---------------------------------------------------------------------
void        wndPlot2d::populate_vect2d( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_VECT2D) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    if (pl._graph.size()!=1)
        return;


    JKQTPVectorFieldGraph* vect_graph = (JKQTPVectorFieldGraph*)pl._graph[0];

	if (vect_graph==nullptr)
        return;

	if (vect_graph->getDxColumn()==-1)
        vect_graph->setDxColumn(ds->getColumnNum(QString::fromStdString(pl._dep)));

	if (vect_graph->getDyColumn()==-1)
        vect_graph->setDyColumn(ds->getColumnNum(QString::fromStdString(pl._depy)));

	if ((vect_graph->getXColumn()==-1)||(vect_graph->getYColumn()))
	{
		QString xColumnName = pl._indep_x.empty()? get_var_indep_name("",get_vector_full_name(pl._dep,pl._depy).toStdString(),0) : QString::fromStdString(pl._indep_x);
		QString yColumnName = pl._indep_y.empty()? get_var_indep_name("",get_vector_full_name(pl._dep,pl._depy).toStdString(),1) : QString::fromStdString(pl._indep_y);

		vect_graph->setXYColumns(QPair<int,int>(
			 ds->getColumnNum(xColumnName),
			 ds->getColumnNum(yColumnName)
			));

		vect_graph->setTitle(get_vector_full_name(pl._dep,pl._depy));
	}


/*
    if ((!pl._indep_x.empty())&&(!pl._indep_y.empty()))
    {
        QString xaxis_label = QString::fromStdString(pl._indep_x);
        QString yaxis_label = QString::fromStdString(pl._indep_y);

        plotter->getXAxis()->setAxisLabel(xaxis_label);
        plotter->getYAxis()->setAxisLabel(yaxis_label);
    }
*/
	//plotter->addGraph(vect_graph);

    redraw(pl);
}

//---------------------------------------------------------------------
// Populate vert. arrow (spectrum like)
//---------------------------------------------------------------------
void        wndPlot2d::populate_vertarrows( plot_descriptor pl)
{
    if (pl._ptype!=PTJK_VERTARROWS) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;
    JKQTPlotter* plotter = _plotters[pl._plot_id];
    if (plotter==nullptr)
        return;

    for (size_t g=0; g < pl._graph.size(); g++)
    {
        // Data is already in the datastore, we just need to add the proper X-Y data to the graph
        JKQTPImpulsesVerticalGraph* arrow_graph = (JKQTPImpulsesVerticalGraph*) pl._graph[g];
        if (arrow_graph==nullptr)
            continue;

        if (!pl._indep_x.empty())
        {
            if (arrow_graph->getXColumn()==-1)
                arrow_graph->setXColumn(ds->getColumnNum(QString::fromStdString(pl._indep_x)));
            /*
            QString xaxis_label = plotter->getXAxis()->getAxisLabel();
            xaxis_label += QString::fromStdString(pl._indep_x) + " ";
            plotter->getXAxis()->setAxisLabel(xaxis_label);
            */
        }
        else
            if (arrow_graph->getXColumn()==-1)
                arrow_graph->setXColumn(ds->getColumnNum(get_var_indep_name(pl._indep_x, pl._dep)));

        if (!pl._dep.empty())
        {
            int nrows = get_var_count(pl._dep);
            if (nrows==1)
            {
                QString vname = QString::fromStdString(pl._dep);
                if (arrow_graph->getYColumn()==-1)
                    arrow_graph->setYColumn(ds->getColumnNum(vname));

                if (arrow_graph->getTitle().isEmpty())
                    arrow_graph->setTitle(QString::fromStdString(pl._dep));
            }
            else
            {
                QString name = get_var_name(pl._dep, g);
                if (arrow_graph->getTitle().isEmpty())
                    arrow_graph->setTitle(name);
                if (arrow_graph->getYColumn()==-1)
                    arrow_graph->setYColumn(ds->getColumnNum(name));
            }
        }
    }

    redraw(pl);
}


//---------------------------------------------------------------------
// Update the graphs: create new ones or delete unnecessary if needed
//---------------------------------------------------------------------
void wndPlot2d::update_grahs_per_dep_boxplot(plot_descriptor& pl, JKQTPlotter* plotter)
{
    if (pl._ptype != PTJK_BOXPLOT)
        return;

    if (plotter==nullptr) return;
    // Check size
    // Get the number of graphs (_dep might be 2D)
    const octave_value& val = _workspace->var_value(pl._dep);
    if (val.ndims()!=2)
        return;

    if (pl._graph.size()==2)
        return;

    for (size_t n = 2; n < pl._graph.size(); n++)
    {
        JKQTPPlotElement *plot_elem = (JKQTPPlotElement *)pl._graph[n];

        if (plot_elem==nullptr)
            continue;

        //deleteGraph will delete the plot element too
        plotter->deleteGraph(plot_elem);
        pl._graph[n] = nullptr;
    }
    if (pl._graph.size()==0)
    {
        JKQTPBoxplotVerticalGraph* new_graph = new JKQTPBoxplotVerticalGraph(plotter);
        pl._graph.push_back(new_graph);
        plotter->addGraph(new_graph);
        new_graph->setTitle("vertical Boxplots");
        new_graph->setMedianLineColor(new_graph->getLineColor().darker());
    }

    if (pl._graph.size()==1)
    {
        JKQTPXYLineGraph* graphOutliers= new JKQTPXYLineGraph(plotter);
        pl._graph.push_back(graphOutliers);
        graphOutliers->setTitle("outliers");
        // make the color a darker shade of the color of graph
        graphOutliers->setColor( ((JKQTPBoxplotVerticalGraph*)(pl._graph[0]))->getLineColor().darker());
        graphOutliers->setSymbolFillColor(QColor("white"));
        // draw outliers as small circles, without lines
        graphOutliers->setSymbolType(JKQTPCircle);
        graphOutliers->setDrawLine(false);
        graphOutliers->setSymbolSize(7);
        plotter->addGraph(graphOutliers);
    }

    redraw(pl);
}

//---------------------------------------------------------------------
// Update the graphs: create new ones or delete unnecessary if needed
//-------------------------------------------------------------------
void wndPlot2d::update_graphs_vector(plot_descriptor& pl, JKQTPlotter* plotter)
{
    if (pl._ptype != PTJK_VECT2D)
        return;

    if (plotter==nullptr) return;

    if (pl._graph.size()>1)
    {
        for (int n = 1; n < pl._graph.size(); n++)
            if (pl._graph[n]!=nullptr)
                plotter->deleteGraph((JKQTPPlotElement*)pl._graph[n]);

        pl._graph.resize(1);
        return;
    }

    if (pl._graph.size()==0)
    {
        JKQTPVectorFieldGraph* graph_vector = new JKQTPVectorFieldGraph(plotter);
        pl._graph.push_back( graph_vector);
        graph_vector->setVectorLengthMode(JKQTPVectorFieldGraph::AutoscaleLength);
        graph_vector->setVectorLineWidthMode(JKQTPVectorFieldGraph::AutoscaleLineWidthFromLength);
		plotter->addGraph(graph_vector);
    }
    redraw(pl);
}

//---------------------------------------------------------------------
// Update the graphs: create new ones or delete unnecessary if needed
//-------------------------------------------------------------------
void wndPlot2d::update_graphs_density(plot_descriptor& pl, JKQTPlotter* plotter)
{
    if (pl._ptype != PTJK_DENS)
        return;

    if (plotter==nullptr) return;

    if (pl._graph.size()>1)
    {
        for (int n = 1; n < pl._graph.size(); n++)
            if (pl._graph[n]!=nullptr)
                plotter->deleteGraph((JKQTPPlotElement*)pl._graph[n]);

        pl._graph.resize(1);
        return;
    }

    if (pl._graph.size()==0)
    {
        JKQTPColumnMathImage* graph_vector = (JKQTPColumnMathImage*)create_graph(pl,plotter);
        pl._graph.push_back( graph_vector);
		plotter->addGraph(graph_vector);
    }

    redraw(pl);
}
//---------------------------------------------------------------------
// Update the graphs: create new ones or delete unnecessary if needed
//---------------------------------------------------------------------
void wndPlot2d::update_graphs_per_dep(plot_descriptor& pl, JKQTPlotter* plotter)
{
    if (pl._plot_id == PTJK_BOXPLOT) return;

    if (plotter==nullptr) return;
    // Check size
    // Get the number of graphs (_dep might be 2D)
    const octave_value& val = _workspace->var_value(pl._dep);

    if ((val.ndims()<1)||(val.ndims()>2)) return;

    int nd0 = val.dims()(0);
    int nd1 = val.dims()(1);

    size_t number_graphs = size_t(nd0 < nd1 ? nd0 : nd1);

	if (number_graphs != pl._graph.size())
	{

		// Delete what is not needed
		for (size_t n=number_graphs; n < pl._graph.size(); n++)
		{
			JKQTPPlotElement *plot_elem = (JKQTPPlotElement *)pl._graph[n];

			if (plot_elem==nullptr)
				continue;

			//deleteGraph will delete the plot element too
			plotter->deleteGraph(plot_elem);
			pl._graph[n] = nullptr;

		}
		if (number_graphs<pl._graph.size())
		{
			pl._graph.resize(number_graphs);
			return;
		}

		// Create graphs not present
		int n0 = pl._graph.size();
		for (size_t n = n0 ; n < number_graphs; n++)
		{
			JKQTPPlotElement* new_graph = create_graph(pl, plotter);

			plotter->addGraph(new_graph);
			if (new_graph==nullptr)
			{
				_last_error = "Couldn't create graph";
				qDebug() << _last_error;
				return;
			}
			pl._graph.emplace_back(new_graph);
		}
	}

    redraw(pl);

}
//---------------------------------------------------------------------
// Get the number of curves per a given variable (useful when plotting
// a 2D data)
//---------------------------------------------------------------------
int wndPlot2d::get_var_count(std::string vname)
{
    if (_workspace==nullptr) return 0;

    const octave_value& val = _workspace->var_value(vname);
    if ((val.ndims()<1)||(val.ndims()>2)) return 0;
    int nd0 = val.dims()(0);
    int nd1 = val.dims()(1);
    return nd0 < nd1 ? nd0:nd1;
}

//---------------------------------------------------------------------
// Get the number of points per a given variable (useful when plotting
// a matrix)
//---------------------------------------------------------------------
int wndPlot2d::get_var_length(std::string vname)
{
    if (_workspace==nullptr) return 0;

    const octave_value& val = _workspace->var_value(vname);
    if ((val.ndims()<1)||(val.ndims()>2)) return 0;
    int nd0 = val.dims()(0);
    int nd1 = val.dims()(1);
    return nd0 < nd1 ? nd1:nd0;
}

//---------------------------------------------------------------------
// Build a unique name (esp. useful when a 2D is to be plotted, in that
// case
//---------------------------------------------------------------------
QString wndPlot2d::get_var_name(std::string vname, int order)
{
    if (_workspace==nullptr) return "";
    int var_size = get_var_count(vname);
    if ((var_size==1)||(order<0))
        return QString::fromStdString(vname);

    //if (order >= var_size) return "";
    return QString::fromStdString(vname)+"("+QString::number(order)+")";
}

//---------------------------------------------------------------------
// Build a unique name for the independent variable (if missing x, that
// would be blank and it could not be retrieved).
//---------------------------------------------------------------------
QString wndPlot2d::get_var_indep_name(const std::string& indep, const std::string& dep, size_t dim)
{
    QString comp;
    switch (dim)
    {
    case 0:
        comp = "x"; break;
    case 1:
        comp = "y"; break;
    case 2:
        comp = "z"; break;
    default:
        comp = QString("x")+QString::number(dim);
    }

    if (indep.empty())
        return QString::fromStdString(dep)+"["+comp+"]";
    else
        return QString::fromStdString(indep);
}

//---------------------------------------------------------------------
// Clear all plots
//---------------------------------------------------------------------
void wndPlot2d::clear_plots()
{
    // Recreate
    for (auto &plotter : _plotters)
    {
        if (plotter != nullptr)
            plotter->clearGraphs();
        delete plotter;
    }

    _plotters.clear();
}

//---------------------------------------------------------------------
// Create new plots
//---------------------------------------------------------------------
void wndPlot2d::create_plots()
{
    if (_workspace == nullptr) return;

    //clear_plots();

    if (_total_plot != _plotters.size())
        _plotters.resize(_total_plot, nullptr);

    for (auto& pl : _plots)
    {
        int nrow = pl._plot_id / _nplot_x;
        int ncol = pl._plot_id % _nplot_x;

        // Get the plotter window
        JKQTPlotter* current_plotter = _plotters[pl._plot_id]; //(JKQTPlotter*)_layout->itemAtPosition(nrow,ncol)->widget();

        if (current_plotter==nullptr)
        {
            JKQTPDatastore* ds=get_ds();

            if (ds==nullptr)
                current_plotter = new JKQTPlotter(ui->mainWidget);
            else
                current_plotter =new JKQTPlotter(false, ui->mainWidget, ds);

            _plotters[pl._plot_id] = current_plotter;
            current_plotter->setPlotUpdateEnabled(true);

            _layout->addWidget(current_plotter, nrow,ncol);

            if (current_plotter == nullptr)
            {
                _last_error = "Couldn't create new plot widget";
                qDebug() << _last_error;
                return;
            }
        }
        if (pl._ptype == PTJK_BOXPLOT)
        {
            update_grahs_per_dep_boxplot(pl,current_plotter);
            continue;
        }

        if (pl._ptype == PTJK_VECT2D)
        {
            update_graphs_vector(pl,current_plotter);
            continue;
        }

        if (pl._ptype == PTJK_DENS)
        {
            update_graphs_density(pl,current_plotter);
            continue;
        }

        update_graphs_per_dep(pl,current_plotter);
    }
}

//---------------------------------------------------------------------
// Create new graph element (curves etc)
//---------------------------------------------------------------------
JKQTPGraph*    wndPlot2d::create_graph(plot_descriptor& pl, JKQTPlotter* plotter)
{
    switch (pl._ptype)
    {
    case PTJK_PLOT:
    {
        JKQTPXYLineGraph* lg = new JKQTPXYLineGraph(plotter);
        if (lg==nullptr) return nullptr;
        lg->setSymbolType(JKQTPNoSymbol);
        return lg;
    }
    case PTJK_BOXPLOT:
        return new JKQTPBoxplotVerticalGraph(plotter);
    case PTJK_AREA:
    {

        JKQTPFilledCurveXGraph* filled_plot_graph = new JKQTPFilledCurveXGraph(plotter);
        if (filled_plot_graph==nullptr)
            return nullptr;
        QColor col = filled_plot_graph->getFillColor();
        col.setAlphaF(0.25);
        filled_plot_graph->setFillColor(col);
    }
    case PTJK_BARV:
        return new JKQTPBarVerticalGraph(plotter);
    case PTJK_SCATTER:
        return new JKQTPXYScatterGraph(plotter);
    case PTJK_DENS:
    {
        JKQTPColumnMathImage* dens_graph =  new JKQTPColumnMathImage(plotter);
        dens_graph->setColorPalette(JKQTPMathImageMATLAB);
        dens_graph->setAutoImageRange(true);
        return dens_graph;
    }
    case PTJK_VECT2D:
    {
        JKQTPVectorFieldGraph* vect_graph = new JKQTPVectorFieldGraph(plotter);
        vect_graph->setVectorLengthMode(JKQTPVectorFieldGraph::AutoscaleLength);
        vect_graph->setVectorLineWidthMode(JKQTPVectorFieldGraph::AutoscaleLineWidthFromLength);
        return vect_graph;
    }
    case PTJK_VERTARROWS:
    {
        JKQTPImpulsesVerticalGraph* arrow_graph = new JKQTPImpulsesVerticalGraph(plotter);
        if (arrow_graph==nullptr) return nullptr;

        arrow_graph->setDrawSymbols(true);
        arrow_graph->setSymbolType(JKQTPFilledTriangle);
        return arrow_graph;

    }
    default:
        return nullptr;

    }
    return nullptr;
}

//---------------------------------------------------------------------
// Populate all plots and update
//---------------------------------------------------------------------
void wndPlot2d::update_plots()
{
    // data is already updated
    for (auto& pl : _plots)
    {
        switch (pl._ptype)
        {
        case PTJK_PLOT:
            populate_plot(pl); break;
        case PTJK_BOXPLOT:
            populate_boxplot(pl); break;
        case PTJK_AREA:
            populate_area(pl); break;
        case PTJK_BARV:
            populate_barv(pl); break;
        case PTJK_SCATTER:
            populate_scatter(pl); break;
        case PTJK_DENS:
            populate_dens(pl); break;
        case PTJK_VECT2D:
            populate_vect2d(pl); break;
        case PTJK_VERTARROWS:
            populate_vertarrows(pl); break;
        case PTJK_CONTOUR:
            populate_vertarrows(pl); break;
        default:
            break;
        }
    }
}

//---------------------------------------------------------------------
// Add a new plot-descriptor
//---------------------------------------------------------------------
int wndPlot2d::add_plot(const QString& var, const QString& x, int plot_id)
{
    PLOT_TYPE pltype = PTJK_PLOT;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var.isEmpty()) return -1;

    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }


    plot_descriptor pl;
    pl._ptype = pltype;
    pl._indep_x = x.toStdString();
    pl._indep_y = "";
    pl._indep_z = "";
    pl._dep=var.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();
    pl._init_zoom = false;

    _plots.emplace_back(pl);
    // Create plots. The datastore is the
    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    if (pl._indep_x.empty())
        default_indep_vect(pl._indep_x, pl._dep);
    else
        octave_to_plotdata(pl._indep_x);
    if (!_last_error.isEmpty())
        return -1;

    octave_to_plotdata(pl._dep);
    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;
}


//---------------------------------------------------------------------
// Add a new scatterplot-descriptor
//---------------------------------------------------------------------

int wndPlot2d::add_scatterplot(const QString& var, const QString& x, int plot_id)
{
    PLOT_TYPE pltype = PTJK_SCATTER;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var.isEmpty()) return -1;

    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }


    plot_descriptor pl;
    pl._ptype = pltype;
    pl._indep_x = x.toStdString();
    pl._indep_y = "";
    pl._indep_z = "";
    pl._dep=var.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);
    // Create plots. The datastore is the
    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    if (pl._indep_x.empty())
        default_indep_vect(pl._indep_x, pl._dep);
    else
        octave_to_plotdata(pl._indep_x);
    if (!_last_error.isEmpty())
        return -1;

    octave_to_plotdata(pl._dep);
    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;
}

//---------------------------------------------------------------------
// Add a new boxplot-descriptor
//---------------------------------------------------------------------
int wndPlot2d::add_boxplot(const QString& var, const QString& x, int plot_id)
{
    PLOT_TYPE pltype = PTJK_BOXPLOT;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var.isEmpty()) return -1;

    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }

    plot_descriptor pl;
    pl._ptype = pltype;
    pl._indep_x = x.toStdString();
    pl._indep_y = "";
    pl._indep_z = "";
    pl._dep=var.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);
    // Create plots. The datastore is the
    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    if (pl._indep_x.empty())
        default_indep_box_plotvect(pl._indep_x, pl._dep);
    else
        octave_to_plotdata(pl._indep_x);

    if (!_last_error.isEmpty())
        return -1;

    octave_to_box_plotdata(pl._dep,pl._indep_x);
    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;
}

//---------------------------------------------------------------------
// Add a new vector field-descriptor
//---------------------------------------------------------------------

int wndPlot2d::add_vector_plot(const QString& var_x, const QString& var_y, const QString& x,const QString& y, int plot_id)
{
    PLOT_TYPE pltype = PTJK_VECT2D;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var_x.isEmpty()) return -1;
    if (var_y.isEmpty()) return -1;
    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }

    plot_descriptor pl;
    pl._ptype = pltype;
    // These are swapped since octave is col-major and JT is row-major
    pl._indep_x = y.toStdString();
    pl._indep_y = x.toStdString();
    pl._indep_z = "";
    pl._dep=var_x.toStdString();
    pl._depy=var_y.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);

    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    octave_to_vectordata(var_x.toStdString(),var_y.toStdString(), x.toStdString(),y.toStdString());

    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;

}



//---------------------------------------------------------------------
// Add a new density
//---------------------------------------------------------------------
int wndPlot2d::add_density_plot(const QString& var_x, const QString& x,const QString& y, int plot_id)
{
    PLOT_TYPE pltype = PTJK_DENS;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var_x.isEmpty()) return -1;
    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }

    plot_descriptor pl;
    pl._ptype = pltype;
    // These are swapped since octave is col-major and JT is row-major
    pl._indep_x = y.toStdString();
    pl._indep_y = x.toStdString();
    pl._indep_z = "";
    pl._dep=var_x.toStdString();
    pl._depy="";
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);

    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    octave_to_densitydata(var_x.toStdString(), x.toStdString(),y.toStdString());

    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;

}
//-
//---------------------------------------------------------------------
// Add a new plot-descriptor
//---------------------------------------------------------------------
int wndPlot2d::add_barplot(const QString& var, const QString& x, int plot_id)
{
    PLOT_TYPE pltype = PTJK_BARV;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var.isEmpty()) return -1;

    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }


    plot_descriptor pl;
    pl._ptype = pltype;
    pl._indep_x = x.toStdString();
    pl._indep_y = "";
    pl._indep_z = "";
    pl._dep=var.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);
    // Create plots. The datastore is the
    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    if (pl._indep_x.empty())
        default_indep_vect(pl._indep_x, pl._dep);
    else
        octave_to_plotdata(pl._indep_x);
    if (!_last_error.isEmpty())
        return -1;

    octave_to_plotdata(pl._dep);
    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;
}

void wndPlot2d::remove_plot(QString var)
{
    std::string str_name_str = var.toStdString();
    auto plot_iter = _plots.begin();
    bool bredraw = false;
    while (plot_iter != _plots.end())
    {
        plot_descriptor& plot = *(plot_iter);
        if ((plot._dep == str_name_str)||(plot._depy == str_name_str))
        {
           // clean(plot);
            JKQTPlotter* plotter = _plotters[plot._plot_id];
            if (plotter!=nullptr)
                clean(plotter, plot);

            _plots.erase(plot_iter);
            bredraw = true;
        }
        else
            plot_iter++;
    }

    if (bredraw)
        update_plots();

}
//---------------------------------------------------------------------
// Add a new area-descriptor
//---------------------------------------------------------------------

void wndPlot2d::clean(JKQTPlotter* main_plotter, plot_descriptor& pl)
{
    if (main_plotter == nullptr) return;
    // Remove graphs
    for (size_t g=0; g < pl._graph.size(); g++)
        main_plotter->deleteGraph((JKQTPPlotElement*)(pl._graph[g]));
    pl._graph.clear();
}
//---------------------------------------------------------------------
// Add a new area-descriptor
//---------------------------------------------------------------------
int wndPlot2d::add_areaplot(const QString& var, const QString& x, int plot_id)
{
    PLOT_TYPE pltype = PTJK_AREA;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var.isEmpty()) return -1;

    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }


    plot_descriptor pl;
    pl._ptype = pltype;
    pl._indep_x = x.toStdString();
    pl._indep_y = "";
    pl._indep_z = "";
    pl._dep=var.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);
    // Create plots. The datastore is the
    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    if (pl._indep_x.empty())
        default_indep_vect(pl._indep_x, pl._dep);
    else
        octave_to_plotdata(pl._indep_x);
    if (!_last_error.isEmpty())
        return -1;

    octave_to_plotdata(pl._dep);
    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;
}
//---------------------------------------------------------------------
// Add a new arrow-descriptor
//---------------------------------------------------------------------
int wndPlot2d::add_arrowplot(const QString& var, const QString& x, int plot_id)
{
    PLOT_TYPE pltype = PTJK_VERTARROWS;

    if (plot_id >= _total_plot)
        plot_id = -1;

    if (_workspace==nullptr) return -1;
    if (var.isEmpty()) return -1;

    if (plot_id == -1)
    {
        plot_id  = _total_plot;
        _total_plot++;
        _nplot_x = (int)(ceil(sqrt(_total_plot)));
        _nplot_y = (int)(ceil(_total_plot / _nplot_x));
    }

    plot_descriptor pl;
    pl._ptype = pltype;
    pl._indep_x = x.toStdString();
    pl._indep_y = "";
    pl._indep_z = "";
    pl._dep=var.toStdString();
    pl._options = "";
    pl._plot_id = plot_id;
    pl._graph = std::vector<void*>();

    _plots.emplace_back(pl);
    // Create plots. The datastore is the
    create_plots();
    if (!_last_error.isEmpty())
        return -1;

    if (pl._indep_x.empty())
        default_indep_vect(pl._indep_x, pl._dep);
    else
        octave_to_plotdata(pl._indep_x);
    if (!_last_error.isEmpty())
        return -1;

    octave_to_plotdata(pl._dep);
    if (!_last_error.isEmpty())
        return -1;

    update_plots();

    return plot_id;
}

//---------------------------------------------------------------------
// Return current workspace
//---------------------------------------------------------------------
octavews*   wndPlot2d::get_workspace()
{
    return _workspace;
}

//---------------------------------------------------------------------
// Unlink to previous workspace
//---------------------------------------------------------------------
void        wndPlot2d::remove_workspace()
{
    if (_workspace!=nullptr)
        _workspace->remove_graph(this);

    _workspace = nullptr;
}

//---------------------------------------------------------------------
// Data has been updated
//---------------------------------------------------------------------
void wndPlot2d::update_workspace(octavews* ws)
{
    // Check if the workspace that has been updated is the one
    // to which this graph is attached to
    if (ws!=_workspace) return;

    std::set<std::string> vars_to_update = ws->get_updated_variables();
    for (auto& plot : _plots)
    {
        if ((plot._ptype != PTJK_BOXPLOT)&&(plot._ptype!=PTJK_VECT2D)&&(plot._ptype!=PTJK_DENS)&&(plot._ptype!=PTJK_CONTOUR))
        {
            bool update = false;
            const octave_value& val = ws->var_value(plot._dep);
            dim_vector vdim = val.dims();
            int number_of_dims = val.ndims() - vdim.num_ones();
            std::string vname= plot._dep;

            vname = plot._dep;
            if (!vname.empty())
            {
                clear_prev_var(vname);
                if (vars_to_update.find(vname)!=vars_to_update.end())
                {
                    octave_to_plotdata(vname);
                    update = true;
                }
            }
            vname = plot._indep_x;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                {
                    clear_prev_var(vname);
                    octave_to_plotdata(vname);
                    update = true;
                }
            }
            else
                default_indep_vect(vname, plot._dep);

            if (number_of_dims>1)
            {
                vname= plot._indep_y;
                if (!vname.empty())
                {
                    if (vars_to_update.find(vname)!=vars_to_update.end())
                    {
                        clear_prev_var(vname);
                        octave_to_plotdata(vname);
                        update = true;
                    }
                }
                else
                    default_indep_vect(vname, plot._dep);
            }

            if (number_of_dims > 2)
            {
                vname= plot._indep_z;
                if (!vname.empty())
                {
                    if (vars_to_update.find(vname)!=vars_to_update.end())
                    {
                        clear_prev_var(vname);
                        octave_to_plotdata(vname);
                        update = true;
                    }
                }
                else
                    default_indep_vect(vname, plot._dep);
            }

            if (update)
            {
                update_graphs_per_dep(plot, _plotters[plot._plot_id]);
            }
        } // BoxPlot case
        if (plot._ptype == PTJK_BOXPLOT)
        {
            bool update = false;
            std::string vname = plot._indep_x;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                {
                    clear_prev_var(vname);
                    octave_to_plotdata(vname);
                    update = true;
                }
            }
            else
                default_indep_box_plotvect(vname, plot._dep);

            vname = plot._dep;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                {
                    clear_prev_var(vname);
                    octave_to_box_plotdata(plot._dep, get_var_indep_name(plot._indep_x, plot._dep, 0).toStdString());
                    update = true;
                }
            }

            if (update)
                update_grahs_per_dep_boxplot(plot, _plotters[plot._plot_id]);
        }
        // Quiver case
        if (plot._ptype==PTJK_VECT2D)
        {
            bool update = false;
            std::string vname = plot._indep_x;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }
            vname = plot._indep_y;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }

            vname = plot._dep;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }

            vname = plot._depy;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }

            if (update)
            {
                octave_to_vectordata(plot._dep, plot._depy, plot._indep_y, plot._indep_x);
            }

        }
        // Density case
        if (plot._ptype==PTJK_DENS)
        {
            bool update = false;
            std::string vname = plot._indep_x;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }
            vname = plot._indep_y;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }

            vname = plot._dep;
            if (!vname.empty())
            {
                if (vars_to_update.find(vname)!=vars_to_update.end())
                    update = true;
            }


            if (update)
            {
                octave_to_densitydata(plot._dep, plot._indep_y, plot._indep_x);
            }
        }

        if (plot._ptype==PTJK_CONTOUR)
        {

        }
    }

    update_plots();
}

//---------------------------------------------------------------------
// Clear any Datastore content related to a variable, this is needed
// when, after a workspace update, a variable changes its row-count
//---------------------------------------------------------------------

void wndPlot2d::clear_prev_var(std::string varname)
{
    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;
    if (_workspace==nullptr) return;

    int var_id = ds->getColumnNum(QString::fromStdString(varname));
    // if dim is the same we don't need to erase anything

    int row_size = get_var_count(varname);

    if (var_id != -1)
    {
        // we have found a single var, with a single sized var
        if (row_size==1)
            return;

        // Here we have previous val which was 1D (var_id != -1) and we have
        // currently 2D (nd0>1 || nd1>1)
        ds->deleteColumn(var_id);
        return;
    }
    else
    {
        // Prev data was missing or 2D
        // If current data is 1D (max_row==1), we need to erase ALL prev-data, since
        // we need a simple name with no # suffix
        if (row_size == 1)
            row_size = 0;

        int nrow = row_size;
        while (1)
        {
            int var_id = ds->getColumnNum(get_var_name(varname,nrow));
            if (var_id==-1)
                break;
            ds->deleteColumn(var_id);
            nrow++;
        }
    }
}

//---------------------------------------------------------------------
// Build a default x-axis (0..n-1) when no x-axis is given
//---------------------------------------------------------------------

void wndPlot2d::default_indep_vect_vectorfield(const std::string& indep, const std::string& dep,const std::string& depy, size_t dim)
{
    JKQTPDatastore* ds=get_ds();

    if (ds==nullptr) return;
    if (_workspace == nullptr) return;
    if ((dim!=0)&&(dim!=1)) return;

    QString indep_name = get_var_indep_name(indep,get_vector_full_name(dep,depy).toStdString(), dim);

    int var_id = ds->getColumnNum(indep_name);
    if (var_id == -1)
        var_id = ds->addColumn(indep_name);

    dim_vector dims = _workspace->var_value(dep).dims();

    int length = dims(dim);

    ds->resizeColumn(var_id, length);
    for (int n=0; n < length; n++)
        ds->set(var_id, n, (double)(n));

}

//---------------------------------------------------------------------
// Build a default x-axis (0..n-1) when no x-axis is given
//---------------------------------------------------------------------

void wndPlot2d::default_indep_vect(const std::string& indep, const std::string& dep, size_t dim)
{
    JKQTPDatastore* ds=get_ds();

    if (ds==nullptr) return;


    QString indep_name = get_var_indep_name(indep,dep, dim);

    int var_id = ds->getColumnNum(indep_name);
    if (var_id == -1)
        var_id = ds->addColumn(indep_name);

    int length = get_var_length(dep);
	if (ds->getRows(var_id)!=length)
		ds->resizeColumn(var_id, length);
    for (int n=0; n < length; n++)
        ds->set(var_id, n, (double)(n));

}
//---------------------------------------------------------------------
// Same as above but for boxplots (where matrix rows are always the
// independent axis)
//---------------------------------------------------------------------

void wndPlot2d::default_indep_box_plotvect(const std::string& indep, const std::string& dep)
{
    JKQTPDatastore* ds=get_ds();

    if (ds==nullptr) return;

    if (_workspace==nullptr) return;

    QString indep_name = get_var_indep_name(indep,dep, 0);

    int var_id = ds->getColumnNum(indep_name);
    if (var_id == -1)
        var_id = ds->addColumn(indep_name);

    int length = _workspace->var_value(dep).dims()(0);

	if (ds->getRows(var_id)!=length)
		ds->resizeColumn(var_id, length);

    for (int n=0; n < length; n++)
        ds->set(var_id, n, (double)(n));

}

//---------------------------------------------------------------------
// Copy a variable from octave to the graph datastore
//---------------------------------------------------------------------
void wndPlot2d::octave_to_box_plotdata(std::string varname, std::string indep)
{
    _last_error = "";
    if (varname.empty()) return;
    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;

    if (_workspace==nullptr) return;


    octave_value val = _workspace->var_value(varname);

    dim_vector   dims = val.dims();

    if (val.ndims()!=2)
    {
        _last_error = "Data dimension for plot must be 2";
        return;
    }

    bool is_complex = val.is_complex_matrix() || val.is_complex_scalar();

    NDArray val_real = is_complex ? val.abs().array_value() : val.array_value();

    int nrows    = val.dims()(0);
    int nsamples = val.dims()(1);
    QString basename = QString::fromStdString(varname);
    //------------
    // min
    QString min_name = basename + "{min}";
    int var_id_min = ds->getColumnNum(min_name);

    if (var_id_min == -1)
	{   var_id_min = ds->addColumn(min_name); }

	if (ds->getRows(var_id_min)!=nrows)
	{ ds->resizeColumn(var_id_min, nrows);}
    //------------
    // max
    QString max_name = basename + "{max}";
    int var_id_max = ds->getColumnNum(max_name);

    if (var_id_max == -1)
	{ var_id_max = ds->addColumn(max_name); }

	if (ds->getRows(var_id_max)!=nrows)
	{	ds->resizeColumn(var_id_max, nrows); }
    //------------
    // q25
    QString q25_name = basename + "{Q25}";
    int var_id_q25 = ds->getColumnNum(q25_name);

	if (var_id_q25 == -1)
        var_id_q25 = ds->addColumn(q25_name);

	if (ds->getRows(var_id_q25)!=nrows)
		ds->resizeColumn(var_id_q25, nrows);
    //------------
    // q75
    QString q75_name = basename + "{Q75}";
    int var_id_q75 = ds->getColumnNum(q75_name);

	if (var_id_q75 == -1)
        var_id_q75 = ds->addColumn(q75_name);

	if (ds->getRows(var_id_q75)!=nrows)
		ds->resizeColumn(var_id_q75, nrows);
    //------------
    // q25
	QString mean_name = basename + "{Mean}";
	// Check if qwt_plot stcaz
    int var_id_mean = ds->getColumnNum(mean_name);

    if (var_id_mean == -1)
	{ var_id_mean = ds->addColumn(mean_name); }

	if (ds->getRows(var_id_mean)!=nrows)
	{ ds->resizeColumn(var_id_mean, nrows);}
    //------------
    // q75
    QString median_name = basename + "{Median}";
    int var_id_median = ds->getColumnNum(median_name);

    if (var_id_median == -1)
	{ var_id_median = ds->addColumn(median_name);}

	if (ds->getRows(var_id_mean)!=nrows)
	{ ds->resizeColumn(var_id_median, nrows); }

	NDArray row; row.resize(dim_vector({1, nsamples}));

    //---------------------
    // get indep variable as well, so that we
    // position outliers correctly
    QString outlier_name = basename+"{outliers}";
    QString outlier_x_name = basename+"{outliers_x}";

    int var_id_outlier = ds->getColumnNum(outlier_name);
    int var_id_outlier_x=ds->getColumnNum(outlier_x_name);

    if (var_id_outlier==-1)
        var_id_outlier = ds->addColumn(outlier_name);
    if (var_id_outlier_x==-1)
        var_id_outlier_x = ds->addColumn(outlier_x_name);

    int id_xval = ds->getColumnNum(get_var_indep_name(indep, varname));

    QVector<double> outlier_x;
    QVector<double> outlier;



    for (int r = 0; r < nrows; r++)
    {
        double mean = 0.0;

        double x = id_xval == -1? r : ds->get(id_xval,r);

        //double max  =
        for (int sample = 0; sample < nsamples; sample++)
        {
            double val = val_real(r,sample);
            row(sample) =val;
            mean+=val;
        }
        mean /= (double)(nsamples);


        NDArray quantiles = Quantile(row);

        if (quantiles.numel()==1)
            quantiles.resize(dim_vector(1,3),quantiles(0));

        ds->set(var_id_q25, r, quantiles(0));
        ds->set(var_id_q75, r, quantiles(2));
        ds->set(var_id_median,r, quantiles(1));
        ds->set(var_id_mean,r,mean);

        double iqr = 1.5*(quantiles(2)-quantiles(0));
        // Outlier: out of 3Sigma
        double vmax = quantiles(2)+iqr;
        double vmin = quantiles(0)-iqr;

//        qDebug() << "Avg:" << mean;
//        qDebug() << "Om :" << vmin;
//        qDebug() << "OM :" << vmax;



        for (int sample = 0; sample < nsamples; sample++)
        {

            double val = val_real(r,sample);
            //qDebug() << "-- :" << val;

            if ((val>vmax)||(val<vmin))
            {
//                qDebug() << "outlier : " << val;
                outlier_x << x;
                outlier << val;
                // remove the outliers from the "row" vector
                row(sample)=mean;
            }
        }

        ds->set(var_id_max, r, row.max()(0));
        ds->set(var_id_min, r, row.min()(0));
    }
    ds->setColumnData(var_id_outlier_x, outlier_x);
    ds->setColumnData(var_id_outlier, outlier);
}
//---------------------------------------------------------------------
// Copy a variable from octave to the graph datastore
//---------------------------------------------------------------------

void wndPlot2d::octave_to_plotdata(std::string varname)
{
    _last_error = "";
    if (varname.empty()) return;
    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;

    if (_workspace==nullptr) return;


    octave_value val = _workspace->var_value(varname);

    dim_vector   dims = val.dims();

    if (val.ndims()>2)
    {
        _last_error = "Maximum dimension 2 for plot";
        return;
    }

    int nd0 = val.dims()(0);
    int nd1 = val.dims()(1);

    bool is_complex = val.is_complex_matrix() || val.is_complex_scalar();

    NDArray val_real = val.isstruct() ? NDArray() : (is_complex ? val.abs().array_value() : val.array_value());

    clear_prev_var(varname);

    QString sVarName = QString::fromStdString(varname);

    if ((nd0==1)||(nd1==1))
    {
        int length = nd0 == 1 ? nd1 : nd0;
        // Check if the variable is already present into the column database
        int var_id = ds->getColumnNum(sVarName);

        if (var_id == -1)
            var_id = ds->addColumn(sVarName);

        if (ds->getRows(var_id)!=length)
            ds->resizeColumn(var_id, length);

        for (int n=0; n < length; n++)
            ds->set(var_id, n, val_real(n));

        return;
    }

    int xmax = nd0, ymax = nd1;
    if (nd0 > nd1)
    {
        val_real = val_real.transpose();
        xmax = nd1;
        ymax = nd0;
    }



    for (int row = 0; row < xmax ; row++)
    {
        QString tname = QString::fromStdString(varname) + "(" + QString::number(row) + ")";

        // Check if the variable is already present into the column database
        int var_id = ds->getColumnNum(tname);

        if (var_id == -1)
            var_id = ds->addColumn(tname);

        if (ds->getRows(var_id)!=ymax)
            ds->resizeColumn(var_id, ymax);


        for (int col = 0; col < ymax; col++)
            ds->set(var_id, col, val_real(row,col));
    }

}


QString  wndPlot2d::get_vector_full_name(const std::string& vector_x, const std::string& vector_y)
{
    return QString("<")+QString::fromStdString(vector_x)+","+QString::fromStdString(vector_y)+">";
}
//---------------------------------------------------------------------
// Copy a variable from octave to the graph datastore
//---------------------------------------------------------------------
// NB xname and yname are swapped (Octave is col-major, JT is row-major)
void wndPlot2d::octave_to_vectordata(std::string varname_x, std::string varname_y, std::string yname, std::string xname)
{
    _last_error = "";
    if (varname_x.empty()) return;
    if (varname_y.empty()) return;

    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;

    if (_workspace==nullptr) return;

    NDArray val_x = _workspace->var_value(varname_x).array_value();
    NDArray val_y = _workspace->var_value(varname_y).array_value();

    if (val_x.ndims()!=2)
    {
        _last_error = QString::fromStdString(varname_x) +" component must be a 2D array";
        return;
    }

    if (val_y.ndims()!=2)
    {
        _last_error = QString::fromStdString(varname_x) + " component must be a 2D array";
        return;
    }

    dim_vector   dims_x = val_x.dims();
    dim_vector   dims_y = val_y.dims();


    if ((dims_x(0)!=dims_y(0))||(dims_x(1)!=dims_y(1)))
    {
        _last_error = QString("data dimensions of") +QString::fromStdString(varname_x)+
                      "and "+ QString::fromStdString(varname_y)+ " components do not match";
        return;
    }

    if (!xname.empty())
    {
        dim_vector dims_var_x= _workspace->var_value(xname).dims();

        if ((dims_var_x(0)!=1)&&(dims_var_x(1)!=1))
        {
            _last_error = QString::fromStdString(xname) +" must be a vector";
            return;
        }

        if (dims_var_x(0)*dims_var_x(1) != dims_x(1))
        {
            _last_error = QString::fromStdString(xname) + " size must match data column count";
            return;
        }
    }


    if (!yname.empty())
    {
        dim_vector dims_var_y= _workspace->var_value(yname).dims();

        if ((dims_var_y(0)!=1)&&(dims_var_y(1)!=1))
        {
            _last_error =  QString::fromStdString(yname) +" must be a vector";
            return;
        }

        if (dims_var_y(0)*dims_var_y(1) != dims_x(0))
        {
            _last_error =  QString::fromStdString(yname) +" size must match data row count";
            return;
        }
    }

    // Create a linear 2D grid
    std::string full_name = get_vector_full_name(varname_x, varname_y).toStdString();
    QString name_indep_x = get_var_indep_name(xname, full_name, 0);
    QString name_indep_y = get_var_indep_name(yname, full_name, 1);

    // The two are swapped since, in octave, first dimension is rows
    size_t nCols = dims_x(1), nRows = dims_x(0);

    int stot = nRows*nCols;
    std::pair<size_t,size_t> columnXY;
    int cxy_first = ds->getColumnNum(name_indep_x);
    int cxy_second= ds->getColumnNum(name_indep_y);
    if ((cxy_first==-1)||(cxy_second==-1))
        columnXY = ds->addLinearGridColumns(nCols,0,nCols-1,nRows,0,nRows-1,name_indep_x, name_indep_y);
    else
    {
        columnXY.first = cxy_first;
        columnXY.second= cxy_second;


        if (ds->getRows(cxy_first)!=stot)
            ds->resizeColumn(cxy_first,stot);

        if (ds->getRows(cxy_second)!=stot)
            ds->resizeColumn(cxy_second,stot);
    }

    if ((!xname.empty())&&(!yname.empty()))
    {
        NDArray val_xdata = _workspace->var_value(xname).array_value();
        NDArray val_ydata = _workspace->var_value(yname).array_value();
        // The JTKQ is row major
        for (size_t row = 0, n=0; row < nRows; row++)
            for (size_t col = 0 ; col < nCols; col++, n++)
            {
                ds->set(columnXY.first,  n, val_xdata(col));
                ds->set(columnXY.second, n, val_ydata(row));
            }
    }
    else
    {
        for (size_t row = 0, n=0; row < nRows; row++)
            for (size_t col = 0 ; col < nCols; col++, n++)
            {
                ds->set(columnXY.first,  n, col);
                ds->set(columnXY.second, n, row);
            }
    }


    int var_id_x = ds->getColumnNum(QString::fromStdString(varname_x));
    if (var_id_x==-1)
        var_id_x = ds->addColumn(nCols*nRows, QString::fromStdString(varname_x));
    else
        if (ds->getRows(var_id_x)!=stot)
        ds->resizeColumn(var_id_x, stot);


    int var_id_y = ds->getColumnNum(QString::fromStdString(varname_y));
    if (var_id_y==-1)
        var_id_y = ds->addColumn(nCols*nRows, QString::fromStdString(varname_y));
    else
        if (ds->getRows(var_id_y)!=stot)
            ds->resizeColumn(var_id_y, stot);


    // The JTKQ is row major
    for (int row = 0, n=0; row < nRows; row++)
        for (int col = 0 ; col < nCols; col++, n++)
        {
            ds->set(var_id_x, n, val_x(row,col));
            ds->set(var_id_y, n, val_y(row,col));
        }
}

//---------------------------------------------------------------------
// Copy a variable from octave to the graph datastore
//---------------------------------------------------------------------
// NB xname and yname are swapped (Octave is col-major, JT is row-major)
void wndPlot2d::octave_to_densitydata(std::string varname_x,  std::string yname, std::string xname)
{
    _last_error = "";
    if (varname_x.empty()) return;


    JKQTPDatastore* ds=get_ds();
    if (ds==nullptr) return;

    if (_workspace==nullptr) return;

    const octave_value& data = _workspace->var_value(varname_x);
    NDArray val_x = data.is_complex_matrix() || data.is_complex_scalar() ? data.abs().array_value() : data.array_value();


    if (val_x.ndims()!=2)
    {
        _last_error = QString::fromStdString(varname_x) +" component must be a 2D array";
        return;
    }


    dim_vector   dims_x = val_x.dims();

    if (!xname.empty())
    {
        dim_vector dims_var_x= _workspace->var_value(xname).dims();

        if ((dims_var_x(0)!=1)&&(dims_var_x(1)!=1))
        {
            _last_error = QString::fromStdString(xname) +" must be a vector";
            return;
        }

        if (dims_var_x(0)*dims_var_x(1) != 2)
        {
            _last_error = QString::fromStdString(xname) + " must contain Xmin, Xmax";
            return;
        }
    }

    if (!yname.empty())
    {
        dim_vector dims_var_y= _workspace->var_value(yname).dims();

        if ((dims_var_y(0)!=1)&&(dims_var_y(1)!=1))
        {
            _last_error =  QString::fromStdString(yname) +" must be a vector";
            return;
        }

        if (dims_var_y(0)*dims_var_y(1) != 2)
        {
            _last_error =  QString::fromStdString(yname) +" must contain Ymin, Ymax";
            return;
        }
    }

    size_t nCols = dims_x(1), nRows = dims_x(0);

    int x_id = ds->getColumnNum(get_var_indep_name(xname,varname_x,0));
    if (x_id==-1)
        x_id = ds->addColumn(get_var_indep_name(xname,varname_x,0));

    int y_id = ds->getColumnNum(get_var_indep_name(yname,varname_x,1));
	if (y_id==-1)
        y_id = ds->addColumn(get_var_indep_name(yname,varname_x,1));

	if (ds->getRows(x_id)!=3)
		ds->resizeColumn(x_id,3);

	if (ds->getRows(y_id)!=3)
		ds->resizeColumn(y_id,3);
    // Put into column the required values (x|y Min, x|y Max and number of samples)
    if (xname.empty())
    {
        ds->set(x_id,0,0);
        ds->set(x_id,1,nCols-1);
    }
    else
    {
        NDArray x = _workspace->var_value(xname).array_value();
        ds->set(x_id,0,x(0));
        ds->set(x_id,1,x(1));
    }

    ds->set(x_id,2,nCols);


    if (yname.empty())
    {
        ds->set(y_id,0,0);
        ds->set(y_id,1,nRows-1);
    }
    else
    {
        NDArray y = _workspace->var_value(yname).array_value();
        ds->set(y_id,0,y(0));
        ds->set(y_id,1,y(1));
    }

    ds->set(y_id,2,nRows);

    // Put the data into row-major format
    int var_id_x = ds->getColumnNum(QString::fromStdString(varname_x));
    if (var_id_x==-1)
        var_id_x = ds->addImageColumn(nCols, nRows, QString::fromStdString(varname_x));
    else
        if (ds->getRows(var_id_x)!=nRows*nCols)
            ds->resizeColumn(var_id_x, nRows*nCols);


    // The JTKQ is row major
    for (int row = 0, n=0; row < dims_x(0); row++)
        for (int col = 0 ; col < dims_x(1); col++, n++)
            ds->set(var_id_x, n, val_x(row,col));
}


//-------------------------------------------------------------
bool  wndPlot2d::has_var(const QString& str_name)
{
    std::string str_name_str = str_name.toStdString();
    for (const auto& plot:_plots)
    {
        if (plot._dep == str_name_str)   return true;
        if (plot._depy == str_name_str)  return true;

    }
    return false;
}
