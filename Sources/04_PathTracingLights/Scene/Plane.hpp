#pragma once

#include "../Utils.h"

namespace scene
{
    class Plane : public Hittable
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
        Hittable(),position_({0.0f,0.0f,0.0f}),normal_({0.0f,1.0f,0.0f}){}

        /**
         * \brief Основной конструктор
         * \param materialPtr Материал
         * \param position Положение точки на плоскости
         * \param normal Нормаль
         */
        Plane(const std::shared_ptr<materials::Material>& materialPtr, const math::Vec3<float>& position, const math::Vec3<float>& normal):
        Hittable(materialPtr),position_(position),normal_(math::Normalize(normal)){}

        /**
         * \brief Деструктор
         */
        ~Plane() override = default;

        /**
         * \brief Пересечение луча и плоскости
         * \param ray Луч
         * \param tMin Минимальное расстояние
         * \param tMax Максимальное расстояние
         * \param hitInfo Информация о пересечении
         * \return Было ли пересечение с объектом
         */
        bool intersectsRay(const math::Ray& ray, float tMin, float tMax, HitInfo* hitInfo) const override
        {
            // Дистанция до пересечения
            float t = 0;

            // Если пересечение было
            if(ray.intersectsPlane(normal_,position_,tMin,tMax,&t))
            {
                // Если указатель на структуру информации о пересечении был передан
                if(hitInfo != nullptr)
                {
                    // Запись значений
                    hitInfo->t = t;
                    hitInfo->point = ray.getOrigin() + (ray.getDirection() * t);
                    hitInfo->normal = normal_;
                    hitInfo->frontFaceSurface = true;
                    hitInfo->materialPtr = this->materialPtr_;

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
}