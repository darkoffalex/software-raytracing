#pragma once

#include "../Utils.h"

namespace materials
{
    class Metal : public Material
    {
    private:
        /// Собственный цвет материала
        math::Vec3<float> albedo_;
        /// Шероховатость повехрности
        float roughness_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Metal():Material(),albedo_({0.0f,0.0f,0.0f}),roughness_(0.0f){};

        /**
         * \brief Конструктор по умолчанию
         * \param albedo Собственный цвет материала
         */
        explicit Metal(const math::Vec3<float>& albedo = {0.5f,0.5f,0.5f}, float roughness = 0.0f):
        Material(),albedo_(albedo),roughness_(roughness){}

        /**
         * \brief Происходит ли переотражение-разброс лучей для данного материала
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \return Да или нет
         */
        bool isScatters(const math::Ray& rayIn, const HitInfo& hitInfo) const override
        {
            // Данные о входном луче и пересечении не задействованы
            (void) rayIn;
            (void) hitInfo;

            // Материалу свойственен разброс лучей
            return true;
        }

        /**
         * \brief Излучает ли данный материал свет
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \return Да или нет
         */
        bool isEmits(const math::Ray& rayIn, const HitInfo& hitInfo) const override
        {
            // Данные о входном луче и пересечении не задействованы
            (void) rayIn;
            (void) hitInfo;

            // Материал не излучает свет
            return false;
        }

        /**
         * \brief Получить переотраженный-разбросанный луч
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \param attenuationOut Затухание для переотраженного луча
         * \return Переотраженный луч
         */
        math::Ray scatteredRay(const math::Ray& rayIn, const HitInfo& hitInfo, math::Vec3<float>* attenuationOut) const override
        {
            // Данные о входном луче не задействованы
            (void) rayIn;

            // Вектор направления переотраженного луча (отражение луча ровно по нормали)
            math::Vec3<float> scatteredDir = math::Reflect(rayIn.getDirection(),hitInfo.normal);

            // Отраженный от точки пересечения луч распространяется в случайном направлении в пределах полсуферы
            math::Ray scattered(hitInfo.point,RndHemisphereVec(scatteredDir,60.0f * roughness_));

            // Затухание (потеря света) происхолит по закону косинуса
            // Чем угол между нормалью поверхности и вектором отраженного луча меньше - тем сильнее освещенность
            // Также в затухание вносит вклад любое albedo отличное от {1,1,1}
            if(attenuationOut != nullptr)
            {
                float attenuation = std::max(math::Dot(scattered.getDirection(),hitInfo.normal),0.0f);
                *attenuationOut = albedo_ * attenuation;
            }

            return scattered;
        }

        /**
         * \brief Излученный цвет
         * \return Цветовой вектор
         */
        math::Vec3<float> emittedColor() const override
        {
            return {0.0f,0.0f,0.0f};
        }
    };
}