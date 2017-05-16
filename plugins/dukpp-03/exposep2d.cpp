#include "dukpp-03/context.h"

#include <geometry2d.h>
#include <fuzzyequal.h>

#include <p2d/axle.h>
#include <p2d/circle.h>
#include <p2d/line.h>
#include <p2d/rectangle.h>
#include <p2d/collisiontest.h>
#include <p2d/vector.h>
#include <p2d/infiniteline.h>
#include <p2d/body.h>

#include <cassert>

#define PERFORM_AND_ASSERT(X)   {bool b = ctx->eval(X); assert(b); }

static sad::Rect2D ___moveBy(const sad::Point2D & dp, sad::Rect2D & r)
{
    sad::Rect2D result = r;
    sad::moveBy(dp, result);
    return result;
}

static sad::Rect2D __rotateRect(const sad::Rect2D & r, float angle)
{
    sad::Rect2D result = r;
    sad::rotate(result, angle);
    return result;
}

static sad::Vector2D __rotateVector(const sad::Vector2D& r, float angle)
{
    sad::Vector2D result = r;
    sad::rotate(result, angle);
    return result;
}

static bool __collidesR2R(const sad::Rect2D& r1, const sad::Rect2D& r2)
{
    sad::p2d::Rectangle r1x, r2x;
    r1x.setRect(r1);
    r2x.setRect(r2);
    sad::p2d::CollisionTest test1;
    return test1.invoke(&r1x, &r2x);
}

static bool __collidesC2C(const sad::Point2D& c1, double ce1, const sad::Point2D& c2, double ce2)
{
    sad::p2d::Circle c1x, c2x;
    c1x.setCenter(c1);
    c1x.setRadius(ce1);
    c2x.setCenter(c2);
    c2x.setRadius(ce2);
    sad::p2d::CollisionTest test1;
    return test1.invoke(&c1x, &c2x);
}


static bool __collidesL2L(const sad::Point2D& p11, const sad::Point2D& p12, const sad::Point2D& p21, const sad::Point2D& p22)
{
    sad::p2d::Line l1x, l2x;
    l1x.setCutter(sad::p2d::Cutter2D(p11, p12));
    l2x.setCutter(sad::p2d::Cutter2D(p21, p22));
    sad::p2d::CollisionTest test1;
    return test1.invoke(&l1x, &l2x);
}

static sad::p2d::Vector __ortho(const sad::p2d::Vector& v, int i)
{
    if ((i < 0) || (i > 1))
    {
        return v;
    }
    return sad::p2d::ortho(v, static_cast<sad::p2d::OrthoVectorIndex>(i));
}

static bool __isRect2D(const sad::Rect2D& p) {
    return true;
}

// Expose sad::p2d::InfiniteLine and related functions
static void exposeInfiniteLine(sad::dukpp03::Context* ctx)
{
    sad::dukpp03::ClassBinding* c = new sad::dukpp03::ClassBinding();
    c->addConstructor<sad::p2d::InfiniteLine>("SadP2DInfiniteLine");
    c->addConstructor<sad::p2d::InfiniteLine, double, double, double>("SadP2DInfiniteLine");
    c->addCloneValueObjectMethodFor<sad::p2d::InfiniteLine>();

    sad::p2d::MaybePoint (sad::p2d::InfiniteLine::*intersection1)(const sad::p2d::InfiniteLine&) const = sad::p2d::InfiniteLine::intersection;
    sad::p2d::MaybePoint (sad::p2d::InfiniteLine::*intersection2)(const sad::p2d::Cutter2D& a) const = sad::p2d::InfiniteLine::intersection;


    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * intersection_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    intersection_overload->add(sad::dukpp03::bind_method::from(intersection1));
    intersection_overload->add(sad::dukpp03::bind_method::from(intersection1));

    c->addMethod("intersection", intersection_overload);
    c->addMethod("hasPoint", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::hasPoint));
    c->addMethod("isSame", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::isSame));
    c->addMethod("isCollinear", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::isCollinear));
    c->addMethod("kx", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::kx));
    c->addMethod("ky", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::ky));
    c->addMethod("b", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::b));
    c->addMethod("direction", sad::dukpp03::bind_method::from(&sad::p2d::InfiniteLine::direction));

    c->setPrototypeFunction("SadP2DInfiniteLine");

    ctx->addClassBinding("sad::p2d::InfiniteLine", c);

    ctx->registerCallable("SadP2DInfiniteLineFromCutter", sad::dukpp03::make_function::from(sad::p2d::InfiniteLine::fromCutter));

    ctx->registerCallable("SadP2DInfiniteLineAppliedVector", sad::dukpp03::make_function::from(sad::p2d::InfiniteLine::appliedVector));
    
    
    sad::p2d::MaybePoint (*gintersection1)(const sad::p2d::Cutter2D & a, const sad::p2d::Cutter2D & b) = sad::p2d::intersection;
    sad::p2d::MaybePoint (*gintersection2)(const sad::p2d::Point & x, const sad::p2d::Vector & v, const sad::p2d::Cutter2D & c) = sad::p2d::intersection;

    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * global_intersection_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    global_intersection_overload->add(sad::dukpp03::make_function::from(gintersection1));
    global_intersection_overload->add(sad::dukpp03::make_function::from(gintersection2));
    ctx->registerCallable("SadP2DIntersection", global_intersection_overload);

    PERFORM_AND_ASSERT(
        "sad.p2d.InfiniteLine = SadP2DInfiniteLine;"
        "sad.p2d.InfiniteLine.fromCutter = SadP2DInfiniteLineFromCutter;"
        "sad.p2d.InfiniteLine.appliedVector = SadP2DInfiniteLineAppliedVector;"
        "sad.p2d.intersection = SadP2DIntersection;"
        "sad.p2d.InfiniteLine.prototype.toString = function() { return \"sad::p2d::InfiniteLine(\" + this.kx() + \",\" + this.ky() + \",\" + this.b() + \")\";};"
    );
}

// Expose sad::p2d::Body and related functions
static void exposeBody(sad::dukpp03::Context* ctx)
{
    sad::dukpp03::ClassBinding* c = new sad::dukpp03::ClassBinding();
    c->addObjectConstructor<sad::p2d::Body>("SadP2DBody");
    c->addMethod("setUserObject", sad::dukpp03::bind_method::from(&sad::p2d::Body::setUserObject));
    c->addMethod("userObject", sad::dukpp03::bind_method::from(&sad::p2d::Body::userObject));
    c->addMethod("setShape", sad::dukpp03::bind_method::from(&sad::p2d::Body::setShape));
    c->addMethod("currentShape", sad::dukpp03::bind_method::from(&sad::p2d::Body::currentShape));
    c->addMethod("userType", sad::dukpp03::bind_method::from(&sad::p2d::Body::userType));

    c->setPrototypeFunction("SadP2DBody");

    ctx->addClassBinding("sad::p2d::Body", c);

    PERFORM_AND_ASSERT(
        "sad.p2d.Body = SadP2DBody;"
        "sad.p2d.Body.prototype.setCollisionShape = function(o) { return this.setShape(o); };"
        "sad.p2d.Body.prototype.shape = function() { return this.currentShape(); };"
    );
}

void sad::dukpp03::exposeP2D(sad::dukpp03::Context* ctx)
{

    ctx->registerCallable("SadP2DAxle", sad::dukpp03::make_function::from(sad::p2d::axle));

    p2d::Cutter1D (*project1)(const p2d::Cutter2D & c, const p2d::Axle & a) = sad::p2d::project;
    p2d::Cutter1D (*project2)(const sad::Point2D & p1,
        const sad::Point2D & p2,
        const p2d::Axle & a) = sad::p2d::project;

    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * project_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    project_overload->add(sad::dukpp03::make_function::from(project1));
    project_overload->add(sad::dukpp03::make_function::from(project2));
    ctx->registerCallable("SadP2DProject", project_overload);

    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * collides_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    collides_overload->add(sad::dukpp03::make_function::from(sad::p2d::collides));
    collides_overload->add(sad::dukpp03::make_function::from(__collidesR2R));
    collides_overload->add(sad::dukpp03::make_function::from(__collidesC2C));
    collides_overload->add(sad::dukpp03::make_function::from(__collidesL2L));

    ctx->registerCallable("SadP2DCollides", collides_overload);
    ctx->registerCallable("SadP2DCutter", sad::dukpp03::make_function::from(sad::p2d::cutter));

    PERFORM_AND_ASSERT(
        "sad.p2d.axle = SadP2DAxle;"
        "sad.p2d.project = SadP2DProject;"
        "sad.p2d.collides = SadP2DCollides;"
        "sad.p2d.cutter = SadP2DCutter;"
    );

    // Require geometry bindings

    ctx->registerCallable("SadProjectionIsWithin", sad::dukpp03::make_function::from(sad::projectionIsWithin));

    bool (*isWithin1)(const sad::Point2D & p, const sad::Rect2D & r) = sad::isWithin;
    bool (*isWithin2)(const sad::Point2D & p, const sad::Vector<sad::Rect2D> & r) = sad::isWithin;

    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * is_within_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    is_within_overload->add(sad::dukpp03::make_function::from(isWithin1));
    is_within_overload->add(sad::dukpp03::make_function::from(isWithin2));
    ctx->registerCallable("SadIsWithin", is_within_overload);

    ctx->registerCallable("__SadMoveBy", sad::dukpp03::make_function::from(___moveBy));

    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * rotate_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    rotate_overload->add(sad::dukpp03::make_function::from(__rotateRect));
    rotate_overload->add(sad::dukpp03::make_function::from(__rotateVector));
    ctx->registerCallable("__SadRotate", rotate_overload);

    ctx->registerCallable("SadAngleOf", sad::dukpp03::make_function::from(sad::angleOf));
    ctx->registerCallable("SadAcos", sad::dukpp03::make_function::from(sad::acos));
    ctx->registerCallable("SadAsin", sad::dukpp03::make_function::from(sad::asin));
    ctx->registerCallable("SadNormalizeAngle", sad::dukpp03::make_function::from(sad::normalizeAngle));
    ctx->registerCallable("SadFindAngle", sad::dukpp03::make_function::from(sad::findAngle));
    ctx->registerCallable("SadIsValid", sad::dukpp03::make_function::from(sad::isValid));
    ctx->registerCallable("SadIsAABB", sad::dukpp03::make_function::from(sad::isAABB));


    ctx->registerCallable("SadIsFuzzyEqual", sad::dukpp03::make_function::from(sad::is_fuzzy_equal));
    ctx->registerCallable("SadIsFuzzyZero", sad::dukpp03::make_function::from(sad::is_fuzzy_zero));
    ctx->registerCallable("SadNonFuzzyZero", sad::dukpp03::make_function::from(sad::non_fuzzy_zero));

    bool (*eq1)(const sad::Point2D & p1, const sad::Point2D & p2, float precision) = sad::equal;
    bool (*eq2)(const sad::Point3D & p1, const sad::Point3D & p2, float precision) = sad::equal;
    bool (*eq3)(const sad::Rect2D & p1, const sad::Rect2D & p2, float precision) = sad::equal;
    bool (*eq4)(const sad::Rect<sad::Point3D> & p1, const sad::Rect<sad::Point3D> & p2, float precision) = sad::equal;

    ::dukpp03::MultiMethod<sad::dukpp03::BasicContext> * equal_overload = new ::dukpp03::MultiMethod<sad::dukpp03::BasicContext>();
    equal_overload->add(sad::dukpp03::make_function::from(eq1));
    equal_overload->add(sad::dukpp03::make_function::from(eq2));
    equal_overload->add(sad::dukpp03::make_function::from(eq3));
    equal_overload->add(sad::dukpp03::make_function::from(eq4));
    ctx->registerCallable("__SadEqual", equal_overload);

    PERFORM_AND_ASSERT(
        "sad.projectionIsWithin = SadProjectionIsWithin;"
        "sad.isWithin = SadIsWithin;"
        "sad.moveBy = __SadMoveBy;"
        "sad.rotate = __SadRotate;"
        "sad.angleOf = SadAngleOf;"
        "sad.acos = SadAcos;"
        "sad.normalizeAngle = SadNormalizeAngle;"
        "sad.findAngle = SadFindAngle;"
        "sad.isValid = SadIsValid;"
        "sad.isAABB = SadIsAABB;"
        "sad.is_fuzzy_equal = function(x1, x2, prec) { if (typeof prec == \"undefined\") prec = 0.001; return SadIsFuzzyEqual(x1, x2, prec); };"
        "sad.is_fuzzy_zero = function(x, prec) { if (typeof prec == \"undefined\") prec = 0.001; return SadIsFuzzyZero(x, prec); };"
        "sad.non_fuzzy_zero = function(x, prec) { if (typeof prec == \"undefined\") prec = 0.001; return SadNonFuzzyZero(x, prec); };"
        "sad.equal = function(x1, x2, prec) { if (typeof prec == \"undefined\") prec = 0.001; return __SadEqual(x1, x2, prec); };"
    );

    // p2d/vector.h

    ctx->registerCallable("SadP2DModulo", sad::dukpp03::make_function::from(sad::p2d::modulo));
    ctx->registerCallable("SadP2DBasis", sad::dukpp03::make_function::from(sad::p2d::basis));
    ctx->registerCallable("SadP2DUnit", sad::dukpp03::make_function::from(sad::p2d::unit));
    ctx->registerCallable("SadP2DOrtho", sad::dukpp03::make_function::from(__ortho));
    ctx->registerCallable("SadP2DScalar", sad::dukpp03::make_function::from(sad::p2d::scalar));
    ctx->registerCallable("SadP2DRect2D", sad::dukpp03::make_function::from(__isRect2D));


    PERFORM_AND_ASSERT(
        "sad.p2d.OrthoVectorIndex = {};"
        "sad.p2d.OrthoVectorIndex.OVI_DEG_90 = 0;"
        "sad.p2d.OrthoVectorIndex.OVI_DEG_270 = 1;"
        "sad.p2d.modulo = SadP2DModulo;"
        "sad.p2d.basis = SadP2DBasis;"
        "sad.p2d.unit = SadP2DUnit;"
        "sad.p2d.ortho = SadP2DOrtho;"
        "sad.p2d.scalar = SadP2DScalar;"
    );

    exposeInfiniteLine(ctx);
    exposeBody(ctx);
}
