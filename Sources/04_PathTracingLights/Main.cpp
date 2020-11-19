#include <iostream>
#include <functional>
#include <thread>
#include <windows.h>

#include <Math.hpp>
#include <Ray.hpp>
#include <ImageBuffer.hpp>

#include "Scene/Sphere.hpp"
#include "Scene/Plane.hpp"
#include "Scene/Rectangle.hpp"
#include "Scene/Box.hpp"
#include "Materials/Diffuse.hpp"
#include "Materials/Light.hpp"
#include "Materials/Metal.hpp"
#include "Materials/Refractive.hpp"

// Максимальная грубина рекурсии
#define MAX_RECURSION_DEPTH 6
// Мультисемплинг (сколько лучей генерировать на 1 пиксель картинки)
#define SAMPLES_PER_PIXEL 16
// Сколько разбросанных (вторичных) лучей генерировать при пересечении луча и объекта
#define SAMPLES_PER_RAY 1
// Кол-во потоков
#define THREADS 8

/**
 * Коды ошибок
 */
enum ErrorCode
{
    eNoErrors,
    eClassRegistrationError,
    eWindowCreationError,
};

/// Дескриптор исполняемого модуля программы
HINSTANCE g_hInstance = nullptr;
/// Дескриптор осноного окна отрисовки
HWND g_hwnd = nullptr;
/// Дескриптор контекста отрисовки
HDC g_hdc = nullptr;
/// Наименование класса
const char* g_strClassName = "MainWindowClass";
/// Заголовок окна
const char* g_strWindowCaption = "04 - Path tracing light sources";
/// Код последней ошибки
ErrorCode g_lastError = ErrorCode::eNoErrors;

/** W I N A P I  S T U F F **/

/**
 * \brief Обработчик оконных сообщений
 * \param hWnd Дескриптор окна
 * \param message Сообщение
 * \param wParam Параметр сообщения
 * \param lParam Параметр сообщения
 * \return Код выполнения
 */
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * \brief Копирование информации о пикселях изображения в буфер "поверхности" окна
 * \param pixels Массив пикселей
 * \param width Ширина области
 * \param height Высота области
 * \param hWnd Дескриптор окна
 */
void PresentFrame(void *pixels, int width, int height, HWND hWnd);

/** R A Y T R A C I N G  **/

/**
 * \brief Метод рендеринга сцены
 * \param imageBuffer Целевой буфер изображения
 * \param scene Сцена
 * \param fov Угол обзора
 * \param samples Кол-во семплов (лучей) на пиксель буфера
 * \param viewPosition Положение камеры
 * \param viewOrient Ориентация наблюдателя
 *
 * \details В данном методе происходит генерация лучей для каждого пикселя кадрового буфера и последующая
 * трассировка лучами сцены, а также запись полученных значений в пиксели кадрового буфера
 */
void Render(
        ImageBuffer<RGBQUAD> *imageBuffer,
        const scene::List& scene,
        const float& fov,
        unsigned samples,
        math::Vec3<float> viewPosition = {0.0f,0.0f,0.0f},
        math::Vec3<float> viewOrient = {0.0f,0.0f,0.0f});

/**
 * \brief Метод трассировки сцены лучом
 * \param ray Луч
 * \param hittableElement Трассируемый элемент
 * \param outColor Результирующий цвет для точки пересечения
 * \param recursionDepth Глубина рекурсии
 * \return Было ли пересечение с каким-либо объектом сцены
 */
bool TraceTay(
        const math::Ray& ray,
        const scene::Hittable& sceneElement,
        math::Vec3<float>* outColor,
        unsigned recursionDepth = 0);

/** M A I N **/

/**
 * \brief Точка входа
 * \param argc Кол-во аргументов
 * \param argv Аргмуенты
 * \return Код исполнения
 */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try {
        // Получение дескриптора исполняемого модуля программы
        g_hInstance = GetModuleHandle(nullptr);

        // Информация о классе
        WNDCLASSEX classInfo;
        classInfo.cbSize = sizeof(WNDCLASSEX);
        classInfo.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        classInfo.cbClsExtra = 0;
        classInfo.cbWndExtra = 0;
        classInfo.hInstance = g_hInstance;
        classInfo.hIcon = LoadIcon(g_hInstance, IDI_APPLICATION);
        classInfo.hIconSm = LoadIcon(g_hInstance, IDI_APPLICATION);
        classInfo.hCursor = LoadCursor(nullptr, IDC_ARROW);
        classInfo.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
        classInfo.lpszMenuName = nullptr;
        classInfo.lpszClassName = g_strClassName;
        classInfo.lpfnWndProc = WindowProcedure;

        // Пытаемся зарегистрировать оконный класс
        if (!RegisterClassEx(&classInfo)) {
            g_lastError = ErrorCode::eClassRegistrationError;
            throw std::runtime_error("ERROR: Can't register window class.");
        }

        // Создание окна
        g_hwnd = CreateWindow(
                g_strClassName,
                g_strWindowCaption,
                WS_OVERLAPPEDWINDOW,
                0, 0,
                800, 600,
                nullptr,
                nullptr,
                g_hInstance,
                nullptr);

        // Если не удалось создать окно
        if (!g_hwnd) {
            g_lastError = ErrorCode::eWindowCreationError;
            throw std::runtime_error("ERROR: Can't create main application window.");
        }

        // Показать окно
        ShowWindow(g_hwnd, SW_SHOWNORMAL);

        // Получение контекста отрисовки
        g_hdc = GetDC(g_hwnd);

        // Размеры клиентской области окна
        RECT clientRect;
        GetClientRect(g_hwnd, &clientRect);

        /** RAYTRACING **/

        // Создать буффер кадра
        auto frameBuffer = ImageBuffer<RGBQUAD>(clientRect.right, clientRect.bottom, {0, 0, 0, 0});
        std::cout << "INFO: Frame-buffer initialized  (resolution : " << frameBuffer.getWidth() << "x" << frameBuffer.getHeight() << ", size : " << frameBuffer.getSize() << " bytes)" << std::endl;

        // Материалы
        auto red = std::make_shared<materials::Diffuse>(math::Vec3<float>(0.65f, 0.05f, 0.05f));
        auto white = std::make_shared<materials::Diffuse>(math::Vec3<float>(0.73f, 0.73f, 0.73f));
        auto green = std::make_shared<materials::Diffuse>(math::Vec3<float>(0.12f, 0.45f, 0.15f));
        auto light = std::make_shared<materials::Light>(math::Vec3<float>(15.0f,15.0f,15.0f));
        auto metal = std::make_shared<materials::Metal>(math::Vec3<float>(0.8f,0.8f,0.8f),0.3f);
        auto ball = std::make_shared<materials::Diffuse>(math::Vec3<float>(0.1f,0.2f,0.5f));
        auto glass = std::make_shared<materials::Refractive>(0.6f);

        // Сцена
        scene::List scene{};
        scene.addElement(std::make_shared<scene::Plane>(white,math::Vec3<float>(0.0f,5.0f,0.0f),math::Vec3<float>(0.0f,-1.0f,0.0f)));
        scene.addElement(std::make_shared<scene::Plane>(white,math::Vec3<float>(0.0f,-5.0f,0.0f),math::Vec3<float>(0.0f,1.0f,0.0f)));
        scene.addElement(std::make_shared<scene::Plane>(white,math::Vec3<float>(0.0f,0.0f,-5.0f),math::Vec3<float>(0.0f,0.0f,1.0f)));
        scene.addElement(std::make_shared<scene::Plane>(red,math::Vec3<float>(-5.0f,0.0f,0.0f),math::Vec3<float>(1.0f,0.0f,0.0f)));
        scene.addElement(std::make_shared<scene::Plane>(green,math::Vec3<float>(5.0f,0.0f,0.0f),math::Vec3<float>(-1.0f,0.0f,0.0f)));
        scene.addElement(std::make_shared<scene::Sphere>(metal,math::Vec3<float>(0.0f,-3.0f,-1.0),2.0f));
        scene.addElement(std::make_shared<scene::Sphere>(ball,math::Vec3<float>(-2.0f,-4.0f,2.5),1.0f));
        scene.addElement(std::make_shared<scene::Sphere>(glass,math::Vec3<float>(2.5f,-3.5f,3.0f),1.5f));
        scene.addElement(std::make_shared<scene::Rectangle>(light,math::Vec3<float>(0.0f,4.95f,0.0f),math::Vec2<float>(3.0f,3.0f), math::Vec3<float>(90.0f,0.0f,0.0f)));

        // Трассировка сцены лучами, запись результата в буфер изображения
        auto renderBeginTime = std::chrono::system_clock::now();
        Render(&frameBuffer, scene, 90.0f, SAMPLES_PER_PIXEL,{0.0f,0.0f,10.0f},{0.0f,0.0f,0.0f});
        std::cout << "INFO: Scene rendered in " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - renderBeginTime).count() << " ms." << std::endl;

        // Показ кадра
        PresentFrame(frameBuffer.getData(), static_cast<int>(frameBuffer.getWidth()), static_cast<int>(frameBuffer.getHeight()), g_hwnd);

        /** MAIN LOOP **/

        // Оконное сообщение
        MSG msg = {};

        // Запуск цикла
        while (true)
        {
            // Обработка оконных сообщений
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                DispatchMessage(&msg);

                if (msg.message == WM_QUIT) {
                    break;
                }
            }
        }
    }
    catch(std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    // Уничтожение окна
    DestroyWindow(g_hwnd);
    // Вырегистрировать класс окна
    UnregisterClass(g_strClassName, g_hInstance);

    // Код выполнения/ошибки
    return static_cast<int>(g_lastError);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** W I N A P I  S T U F F **/

/**
 * \brief Обработчик оконных сообщений
 * \param hWnd Дескриптор окна
 * \param message Сообщение
 * \param wParam Параметр сообщения
 * \param lParam Параметр сообщения
 * \return Код выполнения
 */
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_DESTROY){
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

/**
 * \brief Копирование информации о пикселях изображения в буфер "поверхности" окна
 * \param pixels Массив пикселей
 * \param width Ширина области
 * \param height Высота области
 * \param hWnd Дескриптор окна
 */
void PresentFrame(void *pixels, int width, int height, HWND hWnd)
{
    // Получить хендл на временный bit-map (4 байта на пиксель)
    HBITMAP hBitMap = CreateBitmap(
            width,
            height,
            1,
            8 * 4,
            pixels);

    // Получить device context окна
    HDC hdc = GetDC(hWnd);

    // Временный DC для переноса bit-map'а
    HDC srcHdc = CreateCompatibleDC(hdc);

    // Связать bit-map с временным DC
    SelectObject(srcHdc, hBitMap);

    // Копировать содержимое временного DC в DC окна
    BitBlt(
            hdc,                                  // HDC назначения
            0,                                 // Начало вставки по оси X
            0,                                 // Начало вставки по оси Y
            static_cast<int>(width),              // Ширина
            static_cast<int>(height),             // Высота
            srcHdc,                               // Исходный HDC (из которого будут копироваться данные)
            0,                                // Начало считывания по оси X
            0,                                // Начало считывания по оси Y
            SRCCOPY                               // Копировать
    );

    // Уничтожить bit-map
    DeleteObject(hBitMap);
    // Уничтожить временный DC
    DeleteDC(srcHdc);
    // Уничтожить DC
    ReleaseDC(hWnd,hdc);
}

/** R A Y T R A C I N G  M E T H O D S **/

/**
 * \brief Метод рендеринга сцены
 * \param imageBuffer Целевой буфер изображения
 * \param scene Сцена
 * \param fov Угол обзора
 * \param samples Кол-во семплов (лучей) на пиксель буфера
 * \param viewPosition Положение камеры
 * \param viewOrient Ориентация наблюдателя
 *
 * \details В данном методе происходит генерация лучей для каждого пикселя кадрового буфера и последующая
 * трассировка лучами сцены, а также запись полученных значений в пиксели кадрового буфера
 */
void Render(
        ImageBuffer<RGBQUAD> *imageBuffer,
        const scene::List &scene,
        const float &fov,
        unsigned samples,
        math::Vec3<float> viewPosition,
        math::Vec3<float> viewOrient)
{
    // Размеры кадрового буфера
    auto w = static_cast<float>(imageBuffer->getWidth());
    auto h = static_cast<float>(imageBuffer->getHeight());

    // Угол обзора в радианах
    auto fovRadians = static_cast<float>(fov / (180.0f / M_PI));

    // Всего пикселей в буфере
    unsigned totalPixels = imageBuffer->getWidth() * imageBuffer->getHeight();

    // Лямбда - рендериг блока пикселей
    auto renderBunch = [&](unsigned from, unsigned to){
        // Проход по всем пикселям
        for(unsigned i = from; i < to; i++)
        {
            // Получить координаты пикселя
            unsigned row = i / imageBuffer->getWidth();
            unsigned col = i % imageBuffer->getWidth();

            // Итоговый цвет пикселя
            math::Vec3<float> pixelColor = {0.0f, 0.0f, 0.0f};

            // Проход по семплам пикселя
            for(unsigned s = 0; s < samples; s++)
            {
                // Отклонение луча в пределах пикселя
                // В случае мультисемплинга генерируется случайный сдвинг, в противном случае сдвиг устанавливается в центр пикселя
                math::Vec2<float> pixelBias = (samples > 1 ? math::Vec2<float>(RndFloat(), RndFloat()) : math::Vec2<float>(0.5f, 0.5f));

                // Вычислить отклонение луча для текущего пикселя по углу обзора и текущим координатам пикселя
                float x = (2.0f * (static_cast<float>(col) + pixelBias.x) / w - 1.0f) * tanf(fovRadians / 2.0f) * w / h;
                float y = -(2.0f * (static_cast<float>(row) + pixelBias.y) / h - 1.0f) * tanf(fovRadians / 2.0f);

                // Направление луча (с учетом поворота камеры)
                math::Vec3<float> dir = math::GetRotationMat(viewOrient) * math::Vec3<float>(x,y,-1.0f);

                // Создать луч
                math::Ray ray(viewPosition,dir);

                // Трассировка сцены и получение цвета
                math::Vec3<float> sampleColor = {0.0f,0.0f,0.0f};
                TraceTay(ray, scene, &sampleColor);

                // Прибавить к итоговому цвету цвет семпла
                pixelColor = pixelColor + sampleColor;
            }

            // Усреднение цвета
            pixelColor = pixelColor / static_cast<float>(samples);

            // Гамма коррекция (для гаммы в 2.0)
            pixelColor = {
                    std::sqrt(pixelColor.r),
                    std::sqrt(pixelColor.g),
                    std::sqrt(pixelColor.b),
            };

            // Установка цвета
            imageBuffer->setPoint(col,row,{
                    static_cast<BYTE>(math::Clamp(pixelColor.b, 0.0f, 1.0f) * 255.0f),
                    static_cast<BYTE>(math::Clamp(pixelColor.g, 0.0f, 1.0f) * 255.0f),
                    static_cast<BYTE>(math::Clamp(pixelColor.r, 0.0f, 1.0f) * 255.0f),
                    255
            });
        }
    };

    // Кол-во пикселей на 1 поток
    unsigned pixelBunchSize = totalPixels / static_cast<unsigned>(THREADS);

    // Потоки
    std::vector<std::thread> threads{};

    // Запуск нужного кол-ва потоков
    for(unsigned i = 0; i < THREADS; i++)
    {
        // Индекс начального и конечного пикселя
        unsigned from = pixelBunchSize * i;
        unsigned to = pixelBunchSize * (i + 1);

        // Если это последний поток - добавить остаток от деления (на случай если кол-во пикселей ровно не делится на кол-во потоков)
        if(i == (THREADS - 1)) to += (totalPixels % static_cast<unsigned>(THREADS));

        // Запуск потока и добавление его в массив
        threads.emplace_back(renderBunch,from,to);
    }

    // Ожидание завершения потоков
    for(auto& t : threads) t.join();
}

/**
 * \brief Метод трассировки сцены лучом
 * \param ray Луч
 * \param hittableElement Трассируемый элемент
 * \param outColor Результирующий цвет для точки пересечения
 * \param recursionDepth Глубина рекурсии
 * \return Было ли пересечение с каким-либо объектом сцены
 */
bool TraceTay(
        const math::Ray &ray,
        const scene::Hittable &sceneElement,
        math::Vec3<float> *outColor,
        unsigned int recursionDepth)
{
    // Если превышена глубина - отдать черный цвет
    if(recursionDepth > MAX_RECURSION_DEPTH){
        *outColor = {0.0f,0.0f,0.0f};
        return false;
    }

    // Информация о пересечении с объектом
    HitInfo hitInfo{};

    // Если пересечение было
    if(sceneElement.intersectsRay(ray,0.01f,1000.0f,&hitInfo))
    {
        // Если для пересечения есть материал
        if(hitInfo.materialPtr != nullptr)
        {
            // Суммарный цвет всех лучей направленных от точки пересечения
            math::Vec3<float> resultColor = {0.0f,0.0f,0.0f};

            // Если материал в точке пересечения разбрасывает лучи
            if(hitInfo.materialPtr->isScatters(ray,hitInfo))
            {
                // Генерировать заданное кол-во расбросанных лучей
                for(unsigned s = 0; s < SAMPLES_PER_RAY; s++)
                {
                    // Затухание для разбросанного луча
                    math::Vec3<float> attenuation = {0.0f,0.0f,0.0f};
                    // Разбросанный луч
                    auto scatteredRay = hitInfo.materialPtr->scatteredRay(ray,hitInfo,&attenuation);
                    // Цвет полученный в результате трассировки луча
                    math::Vec3<float> scatteredRayColor = {0.0f,0.0f,0.0f};

                    // Трассировка луча
                    TraceTay(scatteredRay,sceneElement,&scatteredRayColor,recursionDepth + 1);

                    // Добавление к результирующему цвету
                    resultColor = resultColor + (attenuation * scatteredRayColor);
                }

                // Усреднение цвета
                resultColor = resultColor / static_cast<float>(SAMPLES_PER_RAY);
            }

            // Если материал в точке пересечения излучает свет
            if(hitInfo.materialPtr->isEmits(ray,hitInfo))
            {
                resultColor = resultColor + hitInfo.materialPtr->emittedColor();
            }

            // Итоговый цвет
            *outColor = resultColor;
        }
        // При отсутствии материала отдать черный цвет
        else
        {
            *outColor = {0.0f,0.0f,0.0f};
        }

        return true;
    }

    // Фоновый цвет
    *outColor = {0.0f,0.0f,0.0f};

    // Пересечение не засчитано
    return false;
}
