#pragma once

#include "Types.h"

class Plane : public SceneElement
{
private:
    /// Положение точки на плоскости
    math::Vec3<float> position_;
    /// Нормаль
    math::Vec3<float> normal_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    Plane():
    SceneElement(),position_({0.0f,0.0f,0.0f}),normal_({0.0f,1.0f,0.0f}){}

    /**
     * \brief Основной конструктор
     * \param material Материал
     * \param position Положение точки на плоскости
     * \param normal Нормаль
     */
    Plane(const Material& material, const math::Vec3<float>& position,  const math::Vec3<float>& normal):
    SceneElement(material),position_(position),normal_(math::Normalize(normal)){}

    /**
     * \brief Деструктор
     */
    ~Plane() override = default;

    /**
     * \brief Пересечение объекта сцены и луча
     * \param ray Луч
     * \param tMin Минимальное расстояние
     * \param tMax Максимальное расстояние
     * \param tOut Итоговое расстояние до точки пересечения
     * \param normalOut Нормаль поверхности в точке пересечени
     * \return Было ли пересечение с объектом
     */
    bool intersectsRay(const math::Ray& ray, float tMin, float tMax, float* tOut, math::Vec3<float>* normalOut) const override
    {
        float t = 0;
        if(ray.intersectsPlane(normal_,position_,tMin,tMax,&t)){
            if(tOut != nullptr) *tOut = t;
            if(normalOut != nullptr) *normalOut = normal_;
            return true;
        }
        return false;
    }
};