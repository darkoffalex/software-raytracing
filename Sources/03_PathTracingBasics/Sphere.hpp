#pragma once

#include "Utils.h"

class Sphere : public HittableElement
{
private:
    /// Положение центра сферы в пространстве
    math::Vec3<float> position_;
    /// Радиус сферы
    float radius_;
    /// Вывернутая сфера
    bool inverted_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    Sphere():
    HittableElement(),position_({0.0f,0.0f,0.0f}),radius_(1.0f),inverted_(false){};

    /**
     * \brief Основной конструктор
     * \param position Положение центра сферы
     * \param radius Радиус сферы
     * \param materialPtr Указатель на материал сферы
     * \param inverted Вывернутая сфера
     */
    Sphere(const math::Vec3<float>& position, const float& radius, const std::shared_ptr<Material>& materialPtr, bool inverted = false):
    HittableElement(materialPtr),position_(position),radius_(radius),inverted_(inverted){}

    /**
     * \brief Деструктор
     */
    ~Sphere() override = default;

    /**
     * \brief Пересечение сферы и луча
     * \param ray Луч
     * \param tMin Минимальное расстояние
     * \param tMax Максимальное расстояние
     * \param tOut Итоговое расстояние до точки пересечения
     * \param normalOut Нормаль поверхности в точке пересечени
     * \return Было ли пересечение с объектом
     */
    bool intersectsRay(const math::Ray& ray, float tMin, float tMax, HitInfo* hitInfo) const override
    {
        // Дистанция до пересечения
        float t = 0;

        // Если пересеченеи было
        if(ray.intersectsSphere(position_,radius_,tMin,tMax,&t))
        {
            // Если указатель на структуру информации о пересечении был передан
            if(hitInfo != nullptr)
            {
                // Запись значений
                hitInfo->t = t;
                hitInfo->point = ray.getOrigin() + (ray.getDirection() * t);
                hitInfo->normal = math::Normalize(hitInfo->point - position_);
                hitInfo->frontFaceSurface = true;
                hitInfo->materialPtr = this->materialPtr_;

                // Если сфера вывернута наизнанку
                if(inverted_){
                    hitInfo->normal = -hitInfo->normal;
                }

                // Если нормаль не направлена против луча, считать что это обратная сторона (и инвертировать нормаль)
                if(math::Dot(-ray.getDirection(),hitInfo->normal) < 0.0f){
                    hitInfo->normal = -hitInfo->normal;
                    hitInfo->frontFaceSurface = false;
                }
            }
            return true;
        }
        return false;
    }
};