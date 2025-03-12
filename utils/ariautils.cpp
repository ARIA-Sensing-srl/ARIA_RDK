#include <complex>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <ov.h>
#include <ariautils.h>
#include <float.h>

QTextStream& operator << (QTextStream &out, const Complex &value)
{
    return out << value.real() << (std::signbit(value.imag()) ? "" : "+") << value.imag() << 'i';
}

NDArray cross_product(NDArray v1, NDArray v2)
{
    NDArray vout(dim_vector(3,1));
    vout(COORD_X) = v1(COORD_Y) * v2(COORD_Z) - v1(COORD_Z) * v2(COORD_Y);
    vout(COORD_Y) = v1(COORD_Z) * v2(COORD_X) - v1(COORD_X) * v2(COORD_Z);
    vout(COORD_Z) = v1(COORD_Y) * v2(COORD_X) - v1(COORD_X) * v2(COORD_Y);
    return vout;
}

double dot(NDArray v1, NDArray v2)
{
    return v1(COORD_X)*v2(COORD_X)+v1(COORD_Y)*v2(COORD_Y)+v1(COORD_Z)*v2(COORD_Z);
}

void normalize(NDArray& v)
{
    double norm = sqrt(dot(v,v));
    v /= norm;
}

QTextStream& operator >> (QTextStream &in, Complex &value)
{
    double im = 0;
    double re = 0;
    value.real(re);
    value.imag(im);
    // read 1st value
    in >> re;
    if (in.status()!=QTextStream::Ok) return in;
    value.real(re);
    //if (!(in >> re)) return in; // ERROR!
    // check whether next char is 'i'
    qint64 pos = in.pos();
    char c; in >> c;
    if (in.status()!=QTextStream::Ok)
        return in;
    // Pure imaginary case
    if (c=='i')
    {
        value.imag(re);
        value.real(0.0);
        return in;
    }
    // We have chars but not i

    in.seek(pos);
    // remember start of imaginary
    in >> im;
    if (in.status()!=QTextStream::Ok)
        return in;

    in >> c;
    if ((in.status()!=QTextStream::Ok) ||(c!='i'))
    {
        value.imag(0.0);
        in.seek(pos);
        return in;
    }
    value.imag(im);
    return in;
}


bool expectChar(QTextStream& in, char cexp)
{
    if (in.atEnd()) return false;
    char c;
    in >> c;
    if (in.status()!=QTextStream::Ok)
        return false;
    return c==cexp;
}

bool readVal(QTextStream& in, QString& out, char endChar)
{
    if (in.atEnd()) return false;
    in >> out;
    if (in.status()!=QTextStream::Ok)
        return false;
    return expectChar(in,endChar);
}


bool readVal(QTextStream& in, quint64& out, char endChar)
{
    if (in.atEnd()) return false;
    in >> out;
    if (in.status()!=QTextStream::Ok)
        return false;
    return expectChar(in,endChar);
}


bool readVal(QTextStream& in, double& out, char endChar)
{
    if (in.atEnd()) return false;
    in >> out;
    if (in.status()!=QTextStream::Ok)
        return false;
    return expectChar(in,endChar);
}

bool readVal(QTextStream& in, Complex& out, char endChar)
{
    if (in.atEnd()) return false;
    in >> out;
    if (in.status()!=QTextStream::Ok)
        return false;
    return expectChar(in,endChar);
}


bool checkString(QTextStream& in, const QString& strCheck)
{
    QString out;
    if (in.atEnd()) return false;
    in >> out;
    if (in.status()!=QTextStream::Ok)
        return false;
    if (!expectChar(in,'\n')) return false;
    return out==strCheck;
}



NDArray     polar_to_cartesian(double r, double az, double zen)
{
    double common = r * sin(zen);
    NDArray cartesian_vector(dim_vector(1,3));
    cartesian_vector(COORD_X) = common * cos(az);
    cartesian_vector(COORD_Y) = common * sin(az);
    cartesian_vector(COORD_Z) = r * cos(zen);
    return cartesian_vector;
}


NDArray     polar_to_cartesian(NDArray polar_vector)
{
    double r    = polar_vector(COORD_R);
    double zen  = polar_vector(COORD_ZEN);
    double az   = polar_vector(COORD_AZ);

    double common = r * sin(zen);
    NDArray cartesian_vector(dim_vector(1,3));
    cartesian_vector(COORD_X) = common * cos(az);
    cartesian_vector(COORD_Y) = common * sin(az);
    cartesian_vector(COORD_Z) = r * cos(zen);
    return cartesian_vector;
}


NDArray     polar_to_cartesian(NDArray polar_vector, NDArray center_cartesian)
{
    double r    = polar_vector(COORD_R);
    double zen  = polar_vector(COORD_ZEN);
    double az   = polar_vector(COORD_AZ);

    double common = r * sin(zen);
    NDArray cartesian_vector(dim_vector(1,3));
    cartesian_vector(COORD_X) = common * cos(az) + center_cartesian(COORD_X);
    cartesian_vector(COORD_Y) = common * sin(az) + center_cartesian(COORD_Y);
    cartesian_vector(COORD_Z) = r * cos(zen)     + center_cartesian(COORD_Z);
    return cartesian_vector;
}
//-----------------------------------------------------
// Given a polar vector w.r.t. (0,0,0), and a center, it calculates
// cartesian coordinates w.r.t. the center
NDArray     polar_to_cartesian(double r, double az, double zen, double xc, double yc, double zc)
{
    double common = r * sin(zen);
    NDArray cartesian_vector(dim_vector(1,3));
    cartesian_vector(COORD_X) = common * cos(az) + xc;
    cartesian_vector(COORD_Y) = common * sin(az) + yc;
    cartesian_vector(COORD_Z) = r * cos(zen)     + zc;
    return cartesian_vector;
}

NDArray     cartesian_to_polar(double x, double y, double z)
{
    double r    = sqrt(x*x+y*y+z*z);
    double zen  = acos(z/r);
    double az   = atan2(y,x);
    NDArray polar_out(dim_vector(1,3));
    polar_out(COORD_R) = r;
    polar_out(COORD_AZ) = az;
    polar_out(COORD_ZEN) = zen;

    return polar_out;
}

NDArray     cartesian_to_polar(NDArray cartesian_vector)
{
    double x = cartesian_vector(COORD_X);
    double y = cartesian_vector(COORD_Y);
    double z = cartesian_vector(COORD_Z);

    double r    = sqrt(x*x+y*y+z*z);
    double zen  = acos(z/r);
    double az   = atan2(y,x);
    NDArray polar_out(dim_vector(1,3));
    polar_out(COORD_R) = r;
    polar_out(COORD_AZ) = az;
    polar_out(COORD_ZEN) = zen;

    return polar_out;
}

NDArray     cartesian_to_polar(NDArray cartesian_vector, NDArray center)
{
    double x = cartesian_vector(COORD_X) - center(COORD_X);
    double y = cartesian_vector(COORD_Y) - center(COORD_Y);
    double z = cartesian_vector(COORD_Z) - center(COORD_Z);

    double r    = sqrt(x*x+y*y+z*z);
    double zen  = acos(z/r);
    double az   = atan2(y,x);
    NDArray polar_out(dim_vector(1,3));
    polar_out(COORD_R) = r;
    polar_out(COORD_AZ) = az;
    polar_out(COORD_ZEN) = zen;

    return polar_out;
}

//-----------------------------------------------------
// Given a vector in R3, and a center, it calculates
// polar coordinates w.r.t. the center
NDArray     cartesian_to_polar(double x, double y, double z, double xc, double yc, double zc)
{
    x-=xc;
    y-=yc;
    z-=zc;
    double r    = sqrt(x*x+y*y+z*z);
    double zen  = acos(z/r);
    double az   = atan2(y,x);
    NDArray polar_out(dim_vector(1,3));
    polar_out(COORD_R) = r;
    polar_out(COORD_AZ) = az;
    polar_out(COORD_ZEN) = zen;

    return polar_out;
}


 void cartesian_to_polar(double x, double y, double z, double* r, double* az, double* zen)
{
    *r    = sqrt(x*x+y*y+z*z);
    *zen  = acos(z/(*r));
    *az   = atan2(y,x);
}


int findNearestNeighbourIndex( float value, NDArray &x )
{
    float dist = FLT_MAX;
    int idx = -1;
    for ( int i = 0; i < x.numel(); ++i ) {
        float newDist = value - x(i);
        if ( newDist > 0 && newDist < dist ) {
            dist = newDist;
            idx = i;
        }
    }

    return idx;
}

NDArray interp1( NDArray &x, NDArray &y, NDArray &x_new )
{
    NDArray y_new;
    y_new.resize( dim_vector(1,x_new.numel()));

    NDArray dx, dy, slope, intercept;
    dx.resize( dim_vector(1, x.numel()));
    dy.resize( dim_vector(1, x.numel()));
    slope.resize( dim_vector(1, x.numel()));
    intercept.resize( dim_vector(1, x.numel()));

    for( int i = 0; i < x.numel(); ++i ){
        if( i < x.numel()-1 )
        {
            dx(i)=( x(i+1) - x(i) );
            dy(i)= ( y(i+1) - y(i) );
            slope(i)=( dy(i) / dx(i) );
            intercept(i)= ( y(i) - x(i) * slope(i) );
        }
        else
        {
            dx(i) = ( dx(i-1) );
            dy(i) = ( dy(i-1) );
            slope(i) = ( slope(i-1) );
            intercept(i)=( intercept(i-1) );
        }
    }

    for ( int i = 0; i < x_new.numel(); ++i )
    {
        int idx = findNearestNeighbourIndex( x_new(i), x );
        y_new(i) = ( slope(idx) * x_new(i) + intercept(idx) );
    }

    return y_new;
}

Matrix     get_rotation_matrix(double rot_x, double rot_y, double rot_z)
{
    Matrix matrix_out(dim_vector(3,3),0);
    // Apply rotation along x, rotation along y and rotation along z

    Matrix matrix_rot_x(dim_vector(3,3),0);
    Matrix matrix_rot_y(dim_vector(3,3),0);
    Matrix matrix_rot_z(dim_vector(3,3),0);
    matrix_rot_z(0,0) =  cos(rot_z);
    matrix_rot_z(0,1) = -sin(rot_z);
    matrix_rot_z(0,2) =  0;

    matrix_rot_z(1,0) =  sin(rot_z);
    matrix_rot_z(1,1) =  cos(rot_z);
    matrix_rot_z(1,2) =  0;

    matrix_rot_z(2,0) =  0;
    matrix_rot_z(2,1) =  0;
    matrix_rot_z(2,2) =  1;

    matrix_rot_x(0,0) =  1;
    matrix_rot_x(0,1) =  0;
    matrix_rot_x(0,2) =  0;

    matrix_rot_x(1,0) =  0;
    matrix_rot_x(1,1) =  cos(rot_x);
    matrix_rot_x(1,2) =  -sin(rot_x);

    matrix_rot_x(2,0) =  0;
    matrix_rot_x(2,1) =  sin(rot_x);
    matrix_rot_x(2,2) =  cos(rot_x);

    matrix_rot_y(0,0) =  cos(rot_y);
    matrix_rot_y(0,1) =  0;
    matrix_rot_y(0,2) =  sin(rot_y);

    matrix_rot_y(1,0) =  0;
    matrix_rot_y(1,1) =  1;
    matrix_rot_y(1,2) =  0;

    matrix_rot_y(2,0) =  -sin(rot_y);
    matrix_rot_y(2,1) =  0;
    matrix_rot_y(2,2) =  cos(rot_y);

    matrix_out = matrix_rot_z * (matrix_rot_y * matrix_rot_x);
    return matrix_out;
}

NDArray     convert_theta_versor_to_cartesian(double theta, double phi)
{
    NDArray vector_out(dim_vector(3,1));
    vector_out(0) =  -cos(theta)*cos(phi);
    vector_out(1) = -cos(theta)*sin(phi);
    vector_out(2) = sin(theta);
    return vector_out;
}
NDArray     convert_phi_versor_to_cartesian(double theta, double phi)
{
    NDArray vector_out(dim_vector(3,1));
    vector_out(0) =  -sin(phi);
    vector_out(1) =   cos(phi);
    vector_out(2) =   0;
    return vector_out;
}


NDArray     convert_theta_versor_to_cartesian(double theta, double phi, NDArray center)
{
    return convert_theta_versor_to_cartesian(theta, phi) + center;
}

NDArray     convert_phi_versor_to_cartesian(double theta, double phi, NDArray center)
{
    return convert_phi_versor_to_cartesian(theta,phi) + center;
}


ComplexNDArray interp1( NDArray &x, ComplexNDArray &y, NDArray &x_new )
{
    ComplexNDArray y_new;
    y_new.resize( dim_vector(1,x_new.numel()));

    NDArray dx;
    ComplexNDArray dy, slope, intercept;
    dx.resize( dim_vector(1, x.numel()));
    dy.resize( dim_vector(1, x.numel()));
    slope.resize( dim_vector(1, x.numel()));
    intercept.resize( dim_vector(1, x.numel()));

    for( int i = 0; i < x.numel(); ++i ){
        if( i < x.numel()-1 )
        {
            dx(i)=( x(i+1) - x(i) );
            dy(i)= ( y(i+1) - y(i) );
            slope(i)=( dy(i) / dx(i) );
            intercept(i)= ( y(i) - x(i) * slope(i) );
        }
        else
        {
            dx(i) = ( dx(i-1) );
            dy(i) = ( dy(i-1) );
            slope(i) = ( slope(i-1) );
            intercept(i)=( intercept(i-1) );
        }
    }

    for ( int i = 0; i < x_new.numel(); ++i )
    {
        int idx = findNearestNeighbourIndex( x_new(i), x );
        y_new(i) = ( slope(idx) * x_new(i) + intercept(idx) );
    }

    return y_new;
}


//------------------------------------------------------------------------------
// Calculate polar coordinate of a vector in a ref system shifted of position
// Input vector is in polar coordinates
NDArray remap_theta_phi_to_offset_polar(NDArray vect_polar, NDArray position, NDArray offset)
{
    // 1. Calculate theta, phi in (0,0,0) refsys
    NDArray polar_v0        = cartesian_to_polar(position);
    NDArray theta_versor_v0 = convert_theta_versor_to_cartesian(polar_v0(COORD_THETA), polar_v0(COORD_PHI));
    NDArray phi_versor_v0   = convert_phi_versor_to_cartesian(  polar_v0(COORD_THETA), polar_v0(COORD_PHI));
    NDArray r_versor0       = cross_product(phi_versor_v0, theta_versor_v0);
    normalize(r_versor0);

    NDArray vect_cartesian = r_versor0*vect_polar(COORD_R) + phi_versor_v0*vect_polar(COORD_PHI) + theta_versor_v0*vect_polar(COORD_THETA);
    return remap_theta_phi_to_offset(vect_cartesian, position, offset);
}
//------------------------------------------------------------------------------
// Calculate polar coordinate of a vector in a ref system shifted of position
// Input vector is into cartesian coordinates
NDArray remap_theta_phi_to_offset(NDArray vect_cartesian, NDArray position, NDArray offset)
{
    // 1. Calculate theta, phi in "offset" centered refsys
    NDArray polar_v1 = cartesian_to_polar(position,offset);
    NDArray theta_versor_v1 = convert_theta_versor_to_cartesian(    polar_v1(COORD_THETA), polar_v1(COORD_PHI));
    NDArray phi_versor_v1   = convert_phi_versor_to_cartesian(      polar_v1(COORD_THETA), polar_v1(COORD_PHI));
    NDArray r_versor        = cross_product(phi_versor_v1, theta_versor_v1);
    normalize(r_versor);

    NDArray vout;
    vout(COORD_R)       = dot(vout,r_versor);
    vout(COORD_PHI)     = dot(vout,phi_versor_v1);
    vout(COORD_THETA)   = dot(vout,theta_versor_v1);

    return vout;
}
//------------------------------------------------------------------------------
//
bool            find_azimuth_zenith(NDArray& azimuth, NDArray& zenith, double az, double zen,
                                    int& a0, int& a1,
                                    int& z0, int& z1,
                                    double &da, double &dz)
{
    zen = zen - 2.0 * M_PI * floor(zen / (2*M_PI));
    if (zen > M_PI)
    {
        zen  = 2.0*M_PI-zen;
        az  += M_PI;
    }
    double theta_min = zenith.min()(0);
    double theta_max = zenith.max()(0);
    double phi_min   = azimuth.min()(0);
    double phi_max   = azimuth.max()(0);

    int kmin = (int)(ceil((theta_min - zen)/(2.0*M_PI)));
    int kmax = (int)(floor((theta_max- zen)/(2.0*M_PI)));
    int k = -1;
    z1 = -1;
    z0 = -1;
    if (kmax>=kmin)
    {
        k = kmin;
        zen = zen + M_PI * 2.0 * k;
        for (int iz=0; iz < zenith.numel(); iz++)
        {
            if ((zenith(iz)>=zen)&&(z1==-1))
                z1 = iz;

            if (zenith(iz)<zen)
                z0 = iz;
        }
        // We have both valid
        if ((z0>=0)&&(z1>=0))
            dz = zenith(z1)-zenith(z0);
        else
        {
            //In one of the two following cases, we are at boundary
            if (z1<0)
            {
                z1 = z0;
                dz = 1.0;
            }
            if (z0 < 0)
            {
                z0 = z1;
                dz = 1.0;
            }
        }
    }
    else //kmax >= kmin
    {
        if (kmax==-1)
        {
            z1 = 0;
            z0 = zenith.numel()-1;
            dz = zenith(z1)-(zenith(z0)-2.0*M_PI);
        }
        else
        {
            z1 = zenith.numel()-1;
            z0 = 0;
            dz = zenith(z0)+2.0*M_PI-zenith(z1);
        }
    }

    kmin = (int)(ceil((phi_min - az)/(2.0*M_PI)));
    kmax = (int)(floor((phi_max- az)/(2.0*M_PI)));
    a0 = -1;
    a1 = -1;
    if (kmax>=kmin)
    {
        k = kmin;
        az = az + M_PI * 2.0 * k;
        for (int ia=0; ia < azimuth.numel(); ia++)
        {
            if ((azimuth(ia)>=az)&&(a1==-1))
                a1 = ia;

            if (azimuth(ia)<az)
                a0 = ia;
        }
        // We have both valid
        if ((a0>=0)&&(a1>=0))
            da = azimuth(a1) - azimuth(a0);
        else
        {
            //In one of the two following cases, we are at boundary
            if (a1<0)
            {
                a1 = a0;
                da = 1.0;
            }
            if (a0 < 0)
            {
                a0 = a1;
                da = 1.0;
            }
        }
    }
    else //kmax >= kmin
    {
        if (kmax==-1)
        {
            a1 = 0;
            a0 = azimuth.numel()-1;
            da = azimuth(a1)-(azimuth(a0)-2.0*M_PI);
        }
        else
        {
            a1 = azimuth.numel()-1;
            a0 = 0;
            da = azimuth(a0)+2.0*M_PI-azimuth(a1);
        }
    }
    return true;
}
//------------------------------------------------------------------------------

bool nearly_equal(
    double a, double b,
    double  epsilon, double abs_th)
// those defaults are arbitrary and could be removed
{
    assert(std::numeric_limits<float>::epsilon() <= epsilon);
    assert(epsilon < 1.f);

    if (a == b) return true;

    auto diff = std::abs(a-b);
    auto norm = std::min((std::abs(a) + std::abs(b)), std::numeric_limits<double>::max());
    // or even faster: std::min(std::abs(a + b), std::numeric_limits<float>::max());
    // keeping this commented out until I update figures below
    return diff < std::max(abs_th, epsilon * norm);
}

//------------------------------------------------------------------------------

bool nearly_equal(
    float a, float b,
    float  epsilon, float abs_th)
// those defaults are arbitrary and could be removed
{
    assert(std::numeric_limits<float>::epsilon() <= epsilon);
    assert(epsilon < 1.f);

    if (a == b) return true;

    auto diff = std::abs(a-b);
    auto norm = std::min((std::abs(a) + std::abs(b)), std::numeric_limits<float>::max());
    // or even faster: std::min(std::abs(a + b), std::numeric_limits<float>::max());
    // keeping this commented out until I update figures below
    return diff < std::max(abs_th, epsilon * norm);
}

//------------------------------------------------------------------------------

bool is_equal(const NDArray& x, const NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (!nearly_equal(x(j),y(j)))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const int8NDArray& x, const int8NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const uint8NDArray& x, const uint8NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const int16NDArray& x, const int16NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

bool is_equal(const uint16NDArray& x, const uint16NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const int32NDArray& x, const int32NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

bool is_equal(const uint32NDArray& x, const uint32NDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const charNDArray& x, const charNDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const FloatNDArray& x, const FloatNDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
        if (x(j)!=y(j))
            return false;
    return true;
}

//------------------------------------------------------------------------------

bool is_equal(const ComplexNDArray& x, const ComplexNDArray& y)
{
    if (x.dims()!=y.dims()) return false;
    for (int j=0; j < x.numel(); j++)
    {
        if (!nearly_equal(x(j).real(),y(j).real()))
            return false;
        if (!nearly_equal(x(j).imag(),y(j).imag()))
            return false;
    }
    return true;
}

//-------------------------------------------------------------------------
template<typename T> bool	 equal_t(const Array<T> &x, const Array<T>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		T xv = x(j);
		T yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}


//-------------------------------------------------------------------------
template<> bool	 equal_t<int8_t>(const Array<int8_t> &x, const Array<int8_t>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		int8_t xv = x(j);
		int8_t yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}

//-------------------------------------------------------------------------
template<> bool	 equal_t<uint8_t>(const Array<uint8_t> &x, const Array<uint8_t>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		uint8_t xv = x(j);
		uint8_t yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}

//-------------------------------------------------------------------------
template<> bool	 equal_t<int16_t>(const Array<int16_t> &x, const Array<int16_t>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		int16_t xv = x(j);
		int16_t yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}

//-------------------------------------------------------------------------
template<> bool	 equal_t<uint16_t>(const Array<uint16_t> &x, const Array<uint16_t>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		uint16_t xv = x(j);
		uint16_t yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}
//-------------------------------------------------------------------------
template<> bool	 equal_t<int32_t>(const Array<int32_t> &x, const Array<int32_t>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		int32_t xv = x(j);
		int32_t yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}

//-------------------------------------------------------------------------
template<> bool	 equal_t<uint32_t>(const Array<uint32_t> &x, const Array<uint32_t>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		uint32_t xv = x(j);
		uint32_t yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}

//-------------------------------------------------------------------------
template<> bool	 equal_t<float>(const Array<float> &x, const Array<float>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		float xv = x(j);
		float yv = y(j);
		if (!nearly_equal(xv,yv)) return false;
	}
	return true;
}
//-------------------------------------------------------------------------
template<> bool	 equal_t<Complex>(const Array<Complex> &x, const Array<Complex>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		Complex xv = x(j);
		Complex yv = y(j);
		if (!nearly_equal(xv.real(),yv.real())) return false;
		if (!nearly_equal(xv.imag(),yv.imag())) return false;
	}
	return true;
}
//-------------------------------------------------------------------------
template<> bool	 equal_t<char>(const Array<char> &x, const Array<char>& y)
{
	if (x.dims()!=y.dims()) return false;
	for (int j=0; j < x.numel(); j++)
	{
		char xv = x(j);
		char yv = y(j);
		if (xv!=yv) return false;
	}
	return true;
}
