#pragma once

#include "../Utils.h"

namespace scene
{
    class Rectangle : public Hittable
    {
    private:
        /// Положение центральной точки (локальный центр) прямоугольника
        math::Vec3<float> position_;
        /// Ориентация в пространстве
        math::Vec3<float> orientation_;
        /// Размеры прямоугольника
        math::Vec2<float> sizes_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Rectangle():
        Hittable(),position_({0.0f,0.0f,0.0f}),orientation_({0.0f,0.0f,0.0f}),sizes_({1.0f,1.0f}){}

        /**
         * \brief Основной конструктор
         * \param materialPtr Материал
         * \param position Положение центра прямоугольника
         * \param sizes Размеры (ширина и высота)
         * \param orientation Ориентация в пространстве
         */
        Rectangle(
                const std::shared_ptr<materials::Material>& materialPtr,
                const math::Vec3<float>& position,
                const math::Vec2<float>& sizes = {1.0f,1.0f},
                const math::Vec3<float>& orientation = {0.0f,0.0f,0.0f}):
        Hittable(materialPtr),position_(position),orientation_(orientation),sizes_(sizes){}

        /**
         * \brief Деструктор
         */
        ~Rectangle() override = default;

        /**
         * \brief Пересечение луча и прямоугольника
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

            // Для того чтобы трансформировать объект (положение, ориентацию) нужно применить обратную трансформацию к лучу
            // T.е сдвигается не сам объект, а луч относительно объекта.
            auto inverseRotation = math::GetRotationMat(-orientation_);

            // Траснформированный луч
            math::Ray transformedRay(
                    inverseRotation * (ray.getOrigin() - position_),
                    inverseRotation * ray.getDirection());

            // Половинные ширина и высота
            float halfWidth = this->sizes_.x / 2.0f;
            float halfHeight = this->sizes_.y / 2.0f;

            // По умолчанию нормаль ориентирована в сторону положительной оси Z
            const math::Vec3<float> normal = {0.0f,0.0f,1.0f};

            // Если было пересечение трансформирвоанного луча и выровненного по плоскости XY прямоугольника
            if(transformedRay.intersectsAARectangleXy(0.0f, -halfWidth, halfWidth, -halfHeight, halfHeight, tMin, tMax, &t))
            {
                // Если указатель на структуру информации о пересечении был передан
                if(hitInfo != nullptr)
                {
                    // Запись значений
                    hitInfo->t = t;
                    hitInfo->point = ray.getOrigin() + (ray.getDirection() * t);
                    hitInfo->normal = normal;
                    hitInfo->frontFaceSurface = true;
                    hitInfo->materialPtr = this->materialPtr_;

                    // Если нормаль не направлена против луча, считать что это обратная сторона (и инвертировать нормаль)
                    if(math::Dot(-transformedRay.getDirection(),hitInfo->normal) < 0.0f){
                        hitInfo->normal = -hitInfo->normal;
                        hitInfo->frontFaceSurface = false;
                    }

                    // Поскольку нормаль считалась в пространстве объекта ее нужно перевести в глобальное пространство
                    hitInfo->normal = math::GetRotationMat(orientation_) * hitInfo->normal;
                }
                return true;
            }
            return false;
        }
    };
}