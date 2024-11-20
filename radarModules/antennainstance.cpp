/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */


#include "antennainstance.h"
#include "ariautils.h"
//-------------------------------------------------_
antennaInstance::~antennaInstance()
{
    // Do not remove the model, since it may be used somewhere else
/*    if (model != nullptr)
        delete  model;
    model = nullptr;*/
}

//-------------------------------------------------_
antennaInstance::antennaInstance(antenna_model_pointer antenna_model) :
    antenna(antenna_model==nullptr?"":"newantenna"),
    model(antenna_model),
    rotation(RowVector(2,0)),
    position(RowVector(3,0)),
    delay(0),
    loss(0),
    _freq_of_interest(-1)
{
    _ant_name = "newantenna";
}
//-------------------------------------------------_
antennaInstance::antennaInstance(antennaInstance &ant) :
    antenna((antenna&)ant)
{
    copy_from(ant);
}


antennaInstance::antennaInstance(const antennaInstance &ant) :
    antenna((const antenna&)ant)
{
    copy_from(ant);
}


//-------------------------------------------------_
octave_value        antennaInstance::toOctave()
{
    octave_value out = antenna::toOctave();
    out.subsref(".", {ovl("antenna_name")}) = _ant_name.toStdString();

    out.subsref(".", {ovl("model_name")}) = model==nullptr?"no model":model->get_name().toStdString();
    out.subsref(".", {ovl("rotation")}) = rotation;
    out.subsref(".", {ovl("position")}) = position;
    out.subsref(".", {ovl("delay")})    = delay;
    out.subsref(".", {ovl("loss")})     = loss;

    return out;
}
//-------------------------------------------------_
bool                antennaInstance::fromOctave(octave_value& val)
{
    // names
    octave_value field  = val.subsref(".", {ovl("antenna_name")});
    if ((!(field.is_string()))||(field.isempty()))
        return false;
    QString tempName = QString::fromStdString(field.string_value());

    // model name
    field = val.subsref(".", {ovl("model_name")});
    if ((field.isempty())||(!field.is_string()))
        return false;

   //QString model_name = QString::fromStdString(field.string_value());

    // rotation
    field = val.subsref(".", {ovl("rotation")});
    if ((field.isempty())||(!field.is_real_matrix()))
        return false;
    octave_value rot = field;

    // position
    field = val.subsref(".", {ovl("position")});
    if ((field.isempty())||(!field.is_real_matrix()))
        return false;
    octave_value pos = field;

    // delay
    field = val.subsref(".", {ovl("delay")});
    if ((field.isempty())||(!field.is_real_scalar()))
        return false;
    octave_value dly = field;

    // position
    field = val.subsref(".", {ovl("loss")});
    if ((field.isempty())||(!field.is_real_scalar()))
        return false;
    octave_value ls  = field;

    // everything is valid so far
    if (!antenna::fromOctave(val)) return false;

    _ant_name = tempName;
    rotation = rot.array_value();
    position = pos.array_value();
    delay    = dly.array_value()(0);
    loss     = ls.array_value()(0);

    return true;
}
//-------------------------------------------------_
octave_value        antennaInstance::calculateResponse(octave_value azimuth_range, octave_value zenith_range, octave_value time, octave_value input_wfm)
{
    octave_value out;

    return out;
}
//-------------------------------------------------_
antennaInstance&    antennaInstance::recalcFreqs(octave_value freqSupport)
{

    return *this;
}
//-------------------------------------------------
antennaInstance&    antennaInstance::recalcAzimuthZenith(octave_value azimuthSupport, octave_value zenithSupport)
{

    return *this;
}
//-------------------------------------------------
bool                antennaInstance::setTranslationRotationDelayLoss(octave_value position, octave_value rotation, double delay_ns, double loss_db)
{
    return true;
}
//-------------------------------------------------
antennaInstance     antennaInstance::operator +(antennaInstance& ant2) // Combine two antennas in addictive way (e.g. tx1+tx2)
{
    antennaInstance x;
    x.copy_base_data(*this);
    x.ep = ep + ant2.ep;
    x.et = et + ant2.et;
    x._d_p = _d_p + ant2._d_p;
    x._d_t = _d_t + ant2._d_t;
    return x;
}
//-------------------------------------------------
void    antennaInstance::mult(const antennaInstance& ant2)
{
    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                _d_p(a,z,f)=_d_p(a,z,f)*ant2._d_p(a,z,f);
                _d_t(a,z,f)=_d_t(a,z,f)*ant2._d_t(a,z,f);
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    _d_p(a,z,f)=_d_p(a,z,f)*ant2._d_p(a,z,f);
                    _d_t(a,z,f)=_d_t(a,z,f)*ant2._d_t(a,z,f);
                }

}
//-------------------------------------------------

void    antennaInstance::add(const antennaInstance& ant2)
{
#ifdef DEBUG_FOI
    qDebug() << "Antenna add : " << QString::number(_freq_of_interest);
#endif

    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                ep(a,z,f) = ep(a,z,f)+ant2.ep(a,z,f);
                et(a,z,f) = et(a,z,f)+ant2.et(a,z,f);
                _d_p(a,z,f)=_d_p(a,z,f)+ant2._d_p(a,z,f);
                _d_t(a,z,f)=_d_t(a,z,f)+ant2._d_t(a,z,f);
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    ep(a,z,f) = ep(a,z,f)+ant2.ep(a,z,f);
                    et(a,z,f) = et(a,z,f)+ant2.et(a,z,f);
                    _d_p(a,z,f)=_d_p(a,z,f)+ant2._d_p(a,z,f);
                    _d_t(a,z,f)=_d_t(a,z,f)+ant2._d_t(a,z,f);
                }
}
void    antennaInstance::copy_from(const antennaInstance& ant2)
{
    resize_as(ant2);
    copy_base_data(ant2);
    _d_p        = ant2._d_p;
    _d_t        = ant2._d_t;
    ep             = ant2.ep;
    et             = ant2.et;
    gain           = ant2.gain;
    _gain_eq       = ant2._gain_eq;
    _freq_of_interest = ant2._freq_of_interest;

}


void    antennaInstance::copy_from(antennaInstance& ant2)
{
    resize_as(ant2);
    copy_base_data(ant2);
    _d_p        = ant2._d_p;
    _d_t        = ant2._d_t;
    ep             = ant2.ep;
    et             = ant2.et;
    gain           = ant2.gain;
    _gain_eq       = ant2._gain_eq;
    _freq_of_interest = ant2._freq_of_interest;
}
//-------------------------------------------------
void antennaInstance::resize_as(antennaInstance& ant2)
{
 //   copy_base_data(ant2);
    freqs.resize(ant2.freqs.dims());
    azimuth.resize(ant2.azimuth.dims());
    zenith.resize(ant2.zenith.dims());

    _d_p.resize(ant2._d_p.dims());
    _d_t.resize(ant2._d_t.dims());

    ep.resize(ant2.ep.dims());
    et.resize(ant2.et.dims());

    gain.resize(ant2.gain.dims());

    _gain_eq.resize(ant2._gain_eq.dims());
}
//-------------------------------------------------
void antennaInstance::calculate_plane_wave()
{
    for (int f=0; f < freqs.numel(); f++)
    {
        double k =2.0 * M_PI * freqs(f) / _C0;
        for (int a=0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                double  phase = -k*dot(polar_to_cartesian(1.0, azimuth(a), zenith(z)), position);
                Complex c_phase(cos(phase),sin(phase));
                ep(a,z,f)*=c_phase;
                et(a,z,f)*=c_phase;
                _d_p(a,z,f)*=c_phase;
                _d_t(a,z,f)*=c_phase;
            }
    }
}
//-------------------------------------------------
void antennaInstance::resize_as(const antennaInstance& ant2)
{
    copy_base_data(ant2);

    _d_p.resize(ant2._d_p.dims());
    _d_t.resize(ant2._d_t.dims());

    ep.resize(ant2.ep.dims());
    et.resize(ant2.et.dims());

    gain.resize(ant2.gain.dims());

    _gain_eq.resize(ant2._gain_eq.dims());
}
//-------------------------------------------------
antennaInstance     antennaInstance::operator ()(double delay) // Apply a time delay to "this" and return the resulting field
{
    antennaInstance x;
    x.resize_as(*this);
#ifdef DEBUG_FOI
    qDebug() << "Antenna delay : " << QString::number(_freq_of_interest);
#endif
    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        double k = -2*M_PI*freqs(f)*delay;
        Complex freq_delay(cos(k),sin(k));
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                x.ep(a,z,f) = ep(a,z,f) * freq_delay;
                x.et(a,z,f) = et(a,z,f) * freq_delay;
                x._d_p(a,z,f)=_d_p(a,z,f)*freq_delay;
                x._d_t(a,z,f)=_d_t(a,z,f)*freq_delay;
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
        {
            double k = -2*M_PI*freqs(f)*delay;
            Complex freq_delay(cos(k),sin(k));
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    x.ep(a,z,f) = ep(a,z,f) * freq_delay;
                    x.et(a,z,f) = et(a,z,f) * freq_delay;
                    x._d_p(a,z,f)=_d_p(a,z,f)*freq_delay;
                    x._d_t(a,z,f)=_d_t(a,z,f)*freq_delay;
                }
        }

    return x;
}

void antennaInstance::apply_phase(double phase)
{
    Complex freq_phase(cos(phase),sin(phase));
    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                ep(a,z,f) = ep(a,z,f) * freq_phase;
                et(a,z,f) = et(a,z,f) * freq_phase;
                _d_p(a,z,f)=_d_p(a,z,f)*freq_phase;
                _d_t(a,z,f)=_d_t(a,z,f)*freq_phase;
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    ep(a,z,f) = ep(a,z,f) * freq_phase;
                    et(a,z,f) = et(a,z,f) * freq_phase;
                    _d_p(a,z,f)=_d_p(a,z,f)*freq_phase;
                    _d_t(a,z,f)=_d_t(a,z,f)*freq_phase;
                }
}
//-------------------------------------------------
void  antennaInstance::apply_delay(double delay)
{
    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        double k = -2*M_PI*freqs(f)*delay;
        Complex freq_delay(cos(k),sin(k));
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                ep(a,z,f) = ep(a,z,f) * freq_delay;
                et(a,z,f) = et(a,z,f) * freq_delay;
                _d_p(a,z,f)=_d_p(a,z,f)*freq_delay;
                _d_t(a,z,f)=_d_t(a,z,f)*freq_delay;
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
        {
            double k = -2*M_PI*freqs(f)*delay;
            Complex freq_delay(cos(k),sin(k));
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    ep(a,z,f) = ep(a,z,f) * freq_delay;
                    et(a,z,f) = et(a,z,f) * freq_delay;
                    _d_p(a,z,f)=_d_p(a,z,f)*freq_delay;
                    _d_t(a,z,f)=_d_t(a,z,f)*freq_delay;
                }
        }

}

//-------------------------------------------------
antennaInstance     antennaInstance::operator [](double phase) // Apply a phase delay to "this" and return the resulting field
{
    antennaInstance x;
    x.resize_as(*this);
    Complex freq_phase(cos(phase),sin(phase));
#ifdef DEBUG_FOI
    qDebug() << "Antenna phase : " << QString::number(_freq_of_interest);
#endif

    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                x.ep(a,z,f) = ep(a,z,f) * freq_phase;
                x.et(a,z,f) = et(a,z,f) * freq_phase;
                x._d_p(a,z,f)=_d_p(a,z,f)*freq_phase;
                x._d_t(a,z,f)=_d_t(a,z,f)*freq_phase;
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    x.ep(a,z,f) = ep(a,z,f) * freq_phase;
                    x.et(a,z,f) = et(a,z,f) * freq_phase;
                    x._d_p(a,z,f)=_d_p(a,z,f)*freq_phase;
                    x._d_t(a,z,f)=_d_t(a,z,f)*freq_phase;
                }

    return x;
}
//-------------------------------------------------
antennaInstance     antennaInstance::operator *(antennaInstance& ant2) // Multiply two antennas (e.g. tx*rx)
{
    antennaInstance x;
    x.resize_as(*this);

#ifdef DEBUG_FOI
    qDebug() << "Antenna add : " << QString::number(_freq_of_interest);
#endif

    if (_freq_of_interest >=0 )
    {
        int f = _freq_of_interest;
        for (int a = 0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                x.ep(a,z,f) = ep(a,z,f) * ant2.ep(a,z,f);
                x.et(a,z,f) = et(a,z,f) * ant2.et(a,z,f);
                x._d_p(a,z,f) = _d_p(a,z,f) * ant2._d_p(a,z,f);
                x._d_t(a,z,f) = _d_t(a,z,f) * ant2._d_t(a,z,f);
            }
    }
    else
        for (int f=0 ; f < freqs.numel(); f++)
            for (int a = 0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    x.ep(a,z,f) = ep(a,z,f) * ant2.ep(a,z,f);
                    x.et(a,z,f) = et(a,z,f) * ant2.et(a,z,f);
                    x._d_p(a,z,f) = _d_p(a,z,f) * ant2._d_p(a,z,f);
                    x._d_t(a,z,f) = _d_t(a,z,f) * ant2._d_t(a,z,f);
                }

    return x;
}
//-------------------------------------------------
void antennaInstance::copy_base_data(antennaInstance& ant2)
{
    set_name(ant2.get_name());

    antenna_file.setFileName(ant2.antenna_file.fileName());
    bModified = ant2.bModified;
    gainCalculated = ant2.gainCalculated;

    model       = ant2.model;
    rotation    = ant2.rotation;
    position    = ant2.position;
    delay       = ant2.delay;
    loss        = ant2.loss;
    _ant_name        = ant2._ant_name;

    freqs          = ant2.freqs;
    azimuth        = ant2.azimuth;
    zenith         = ant2.zenith;
}
//-------------------------------------------------
void antennaInstance::copy_base_data(const antennaInstance& ant2)
{
    set_name(ant2.get_name());

    antenna_file.setFileName(ant2.antenna_file.fileName());
    bModified = ant2.bModified;
    gainCalculated = ant2.gainCalculated;

    model       = ant2.model;
    rotation    = ant2.rotation;
    position    = ant2.position;
    delay       = ant2.delay;
    loss        = ant2.loss;
    _ant_name        = ant2._ant_name;

    freqs          = ant2.freqs;
    azimuth        = ant2.azimuth;
    zenith         = ant2.zenith;
}
//-------------------------------------------------
antennaInstance     antennaInstance::operator = (antennaInstance& ant2)
{
    copy_from(ant2);
    return *this;
}
//-------------------------------------------------
bool antennaInstance::has_same_support(antennaInstance &ant2)
{
    double minEps = 1e-4;
    if (freqs.numel()   !=  ant2.freqs.numel())
        return true;
    if (azimuth.numel() !=  ant2.azimuth.numel())
        return true;
    if (zenith.numel() !=  ant2.zenith.numel())
        return true;

    for (int f=0; f < freqs.numel(); f++)
    {
        if (abs(freqs(f)-ant2.freqs(f))>minEps)
            return false;
    }

    for (int a=0; a < azimuth.numel(); a++)
    {
        if (abs(azimuth(a)-ant2.azimuth(a))>minEps)
            return false;
    }

    for (int z=0; z < zenith.numel(); z++)
    {
        if (abs(zenith(z)-ant2.zenith(z))>minEps)
            return false;
    }

    return true;
}

//-------------------------------------------------
//-------------------------------------------------
void    antennaInstance::set_antenna_model(antenna_model_pointer antenna_model)
{
     model = antenna_model; // antenna_model == nullptr? new antenna() : new antenna(*antenna_model);
    if (antenna_model!=nullptr)
        update_fields_from_model();
}
//-------------------------------------------------
std::shared_ptr<antennaInstance> antennaInstance::clone()
{
    return std::make_shared<antennaInstance>((antennaInstance&)(*this));
}

//-------------------------------------------------
void antennaInstance::update_fields_from_model()
{
    if (model==nullptr) return;
    ep          = model->ep;
    et          = model->et;
    freqs       = model->freqs;
    azimuth     = model->azimuth;
    zenith      = model->zenith;
/*    for (int a = 0; a < azimuth.numel(); a++)
        for (int z = 0; z < zenith.numel(); z++)
            for (int f = 0; f < freqs.numel(); f++)
            {
                et(a,z,f) = loss_d * et(a,z,f) * exp(Complex(0,-2.0 * M_PI * freqs(f) * delay_d));
                ep(a,z,f) = loss_d * ep(a,z,f) * exp(Complex(0,-2.0 * M_PI * freqs(f) * delay_d));
            }
*/
    calculate_directivity_aeff();
    calcGain();
}

//-------------------------------------------------
bool antennaInstance::save_xml(QDomDocument& owner, QDomElement& root)
{
    QDomElement antenna_node = owner.createElement("antenna");
    antenna_node.setAttribute("name",_ant_name);

    antenna_node.setAttribute("model_file",model == nullptr ? "" : model->_filename); // Save only relative path
    antenna_node.setAttribute("position_x", get_position(0));
    antenna_node.setAttribute("position_y", get_position(1));
    antenna_node.setAttribute("position_z", get_position(2));

    antenna_node.setAttribute("rotation_theta", get_rotation(false));
    antenna_node.setAttribute("rotation_phi",   get_rotation(true));

    antenna_node.setAttribute("loss", loss);
    antenna_node.setAttribute("delay", delay);

    root.appendChild(antenna_node);
    return true;
}
//-------------------------------------------------
antenna_pointer antennaInstance::load_xml(QDomDocument& owner, QDomElement& root, QString basedir,antenna_model_pointer ref_model)
{
    if (root.isNull())
        root = owner.firstChildElement("antenna");

    if (root.isNull())
        return antenna_pointer(nullptr);

    antenna_pointer new_antenna =  new antennaInstance();

    new_antenna->_ant_name = root.attribute("name");
    if (new_antenna->_ant_name.isEmpty()) return antenna_pointer(nullptr);

    QString model_file = root.attribute("model_file");

    antenna_model_pointer model_reference = ref_model;

    bool bdelete_model_reference = false;

    if (model_file.isEmpty())
        new_antenna->model = nullptr;
    else
    {
        if (ref_model==nullptr)
        {
            model_reference = new antenna();

            bdelete_model_reference = true;

            if (!model_file.isEmpty())
            {
                if (!model_reference->load(model_file))
                {
                    if (new_antenna!=nullptr)
                        delete new_antenna;

                    return antenna_pointer(nullptr);
                }
            }
            else
            {
                bdelete_model_reference = false;
            }
            new_antenna->model = model_reference;
        }
        else
            new_antenna->model = ref_model;
    }
    bool bOk;
    double x,y,z,rtheta,rphi;
    double loss, delay;
    x = root.attribute("position_x").toDouble(&bOk);

    if (bOk)
        y = root.attribute("position_y").toDouble(&bOk);

    if (bOk)
        z = root.attribute("position_z").toDouble(&bOk);

    if (bOk)
        rtheta = root.attribute("rotation_theta").toDouble(&bOk);

    if (bOk)
        rphi  = root.attribute("rotation_phi").toDouble(&bOk);

    if (bOk)
        loss  = root.attribute("loss").toDouble(&bOk);

    if (bOk)
        delay  = root.attribute("delay").toDouble(&bOk);

    if (!bOk)
    {
        if (bdelete_model_reference)
        {
            if (model_reference!=nullptr)
                delete model_reference;
            model_reference = nullptr;
        }

        if (new_antenna!=nullptr)
            delete new_antenna;
        new_antenna = nullptr;

        return antenna_pointer(nullptr);
    }

    new_antenna->set_loss(loss);
    new_antenna->set_delay(delay);
    new_antenna->set_position(0,x);
    new_antenna->set_position(1,y);
    new_antenna->set_position(2,z);
    new_antenna->set_rotation(false, rtheta);
    new_antenna->set_rotation(true, rphi);

    root = root.nextSiblingElement();
    return new_antenna;
}
//-------------------------------------------------

bool antennaInstance::calculate_over_freq_azimuth_zenith_support(NDArray freq_support, NDArray azimuth_support, NDArray zenith_support)
{
    freqs   = freq_support;
    azimuth = azimuth_support;
    zenith  = zenith_support;
    // Freqs are handled as follows:
    // 1. f < fMin/2 = 0
    // 2. f > fMax+fMin/2 = 0
    // 3. fMin/2 <= f < fMin : apply Window function:
    // xk = (f - fmin/2)/fmin
    // 4. fMax <= f < fMax + fMin/2 :
    // xk = (f - fMax + fMin)/(2*fMin)
    // -> k = a0 - a1 * cos(2*pi*xk) + a2 * cos(4*pi*xk) - a3*cos(6*pi*xk),
    // a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168

    double fmin = model->freqs.min()(0);
    double fmax = model->freqs.max()(0);
    double a0 = 0.35875;
    double a1 = 0.48829;
    double a2 = 0.14128;
    double a3 = 0.01168;

    ep.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()));
    et.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()));

    Complex dep_dt, dep_dp, dep_df;
    Complex det_dt, det_dp, det_df;


    int nf1 = -1, nf0 = -1;
    double df;
    for (int nf = 0; nf < freqs.numel(); nf++)
    {
        double k=0.0;
        double f=freqs(nf);

        if ((f < fmin/2)||(k>fmax+fmin/2))
        {
            k=0.0;
            nf1=nf0=0;
            df = 1.0;
        }
        else
            if ((f>=fmin)&&(f<=fmax))
            {
                k=1.0;
                for (int nf = 0; nf <= model->freqs.numel()-1; nf++)
                    if (model->freqs(nf)>=f)
                        { nf1 = nf; break;}

                if (nf1==0)
                    { nf0=nf1; df = 1.0;}
                else
                    { nf0 = nf1-1; df = model->freqs(nf1)-model->freqs(nf0);}

            }
            else
            {
                double xk=0;
                if ((f>=fmin/2)&&(f<fmin))
                {
                    xk = (f-fmin/2)/fmin;
                    nf1 = nf0 = 0;
                    df = 1.0;
                }
                else
                {
                    xk = (f - fmax + fmin)/(2*fmin);
                    nf1 = nf0 = freqs.numel()-1;
                    df  = 1.0;
                }

                k = a0 - a1 * cos(2*M_PI*xk) + a2 * cos(4*M_PI*xk) - a3*cos(6*M_PI*xk);
            }

        int a0,a1,z0,z1;
        double da;
        double dz;

        for (int na=0; na < azimuth.numel(); na++)
        {
            for (int nz=0; nz < zenith.numel(); nz++)
            {
                find_azimuth_zenith(model->azimuth,model->zenith, azimuth(na), zenith(nz),a0,a1,z0,z1,da,dz);
                if ((a1<0)||(a1 >= azimuth.numel())) qDebug() << "a1 error";
                if ((a0<0)||(a0 >= azimuth.numel())) qDebug() << "a0 error";
                if ((z1<0)||(z1 >= azimuth.numel())) qDebug() << "z1 error";
                if ((z0<0)||(z0 >= azimuth.numel())) qDebug() << "z0 error";

                dep_dp = (model->ep(a1,z0,nf0) - model->ep(a0,z0,nf0))/da;
                dep_dt = (model->ep(a0,z1,nf0) - model->ep(a0,z0,nf0))/dz;
                dep_df = (model->ep(a0,z0,nf1) - model->ep(a0,z0,nf0))/df;

                det_dp = (model->et(a1,z0,nf0) - model->et(a0,z0,nf0))/da;
                det_dt = (model->et(a0,z1,nf0) - model->et(a0,z0,nf0))/dz;
                det_df = (model->et(a0,z0,nf1) - model->et(a0,z0,nf0))/df;

                ep(na,nz,nf) = k * (model->ep(a0,z0,nf0) + dep_dp * (azimuth(na) - model->azimuth(a0)) +
                                    dep_dt * (zenith(nz) - model->zenith(z0)) +
                                    dep_df * (freqs(nf) - model->freqs(nf0)));

                et(na,nz,nf) = k * (model->et(a0,z0,nf0) + det_dp * (azimuth(na) - model->azimuth(a0)) +
                                      det_dt * (zenith(nz) - model->zenith(z0)) +
                                      det_df * (freqs(nf) - model->freqs(nf0)));
            }
        }
    } // For int f
    calculate_directivity_aeff();
    calculate_plane_wave();
    calc_gain_equivalent();
    calcGain();

    return true;
}
//-------------------------------------------------

void antennaInstance::calculate_directivity_aeff()
{
    _d_p.clear();
    _d_t.clear();
    gain.clear();
    _gain_eq.clear();

    _d_p.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()));
    _d_t.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()));
    gain.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()));
    _gain_eq.resize(dim_vector(azimuth.numel(), zenith.numel(), freqs.numel()));

    double dz = zenith(1)-zenith(0);
    double da = azimuth(1)-azimuth(0);
    for (int nf=0; nf < freqs.numel(); nf++)
    {
        // Calculate average power
        double p_avg = 0.0;
        for (int na=0; na < azimuth.numel(); na++)
        {
            for (int nz = 0; nz < zenith.numel(); nz++)
                p_avg+=(std::norm(ep(na,nz,nf)) + std::norm(et(na,nz,nf)))*sin(zenith(nz))*dz*da;
        }
        p_avg /= (4.0 * M_PI);

        double one_over_p_avg = sqrt(1.0 / p_avg);
        for (int na=0; na < azimuth.numel(); na++)
        {
            for (int nz = 0; nz < zenith.numel(); nz++)
            {
                _d_p(na,nz,nf) = (ep(na,nz,nf)) * one_over_p_avg;
                _d_t(na,nz,nf) = (et(na,nz,nf)) * one_over_p_avg;
            }
        }

    }

}

//-------------------------------------------------
void antennaInstance::calc_gain_equivalent()
{
    // D=|_dp|^2 + |_dt|^2
    _gain_eq.clear();
    _gain_eq.resize(_d_p.dims());

#ifdef DEBUG_FOI
    qDebug() << "Antenna calc gain : " << QString::number(_freq_of_interest);
#endif
    if (_freq_of_interest >= 0)
    {
        for (int a =0; a < azimuth.numel(); a++)
            for (int z=0; z < zenith.numel(); z++)
            {
                _gain_eq(a,z,_freq_of_interest) = 10.0*log10(std::norm(_d_p(a,z,_freq_of_interest))+std::norm(_d_t(a,z,_freq_of_interest)));
            }
    }
    else
    {
        for (int f=0; f < freqs.numel(); f++)
            for (int a =0; a < azimuth.numel(); a++)
                for (int z=0; z < zenith.numel(); z++)
                {
                    _gain_eq(a,z,f) = 10.0*log10(std::norm(_d_p(a,z,f))+std::norm(_d_t(a,z,f)));
                }
    }

}

//-------------------------------------------------
NDArray antennaInstance::gain_equivalent()
{
    return _gain_eq;
}

//-------------------------------------------------
bool    antennaInstance::operator == (const antennaInstance& ant)
{
    if (model!=ant.model)
        return false;

    if (!is_equal(rotation,ant.rotation)) return false;
    if (!is_equal(position,ant.position)) return false;
    if (!nearly_equal(delay,ant.delay)) return false;
    if (!nearly_equal(loss, ant.loss)) return false;
    if (_ant_name != ant._ant_name) return false;

    if (antenna::operator==(ant)) return false;

    return true;
}
