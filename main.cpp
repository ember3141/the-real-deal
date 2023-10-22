#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Vector2.hpp>
#include <box2d/box2d.h>
#include <iostream>
#include <string> // for string and to_string()

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// Pixels per meter. Box2D uses metric units, so we need to define a conversion
#define PPM 30.0F
// SFML uses degrees for angles while Box2D uses radians
#define DEG_PER_RAD 57.2957795F

// Box2D world for physics simulation, gravity = 9 m/s^2
b2World world(b2Vec2(0, -11));

//popup setup
int buttonPressCount = 0;
int lastButtonPressCount = 0;
bool startCounting = false;
bool isPopupActive = false;
sf::Clock popupClock;
sf::Clock buttonClock;

sf::Text popupText;
sf::Text debugText;

// A structure with all we need to render a box
struct Box {
    float width;
    float height;
    sf::Color color;
    b2Body* body;
};

// A structure with all we need to render a circle
struct Circle {
    float radius;
    sf::Color color;
    b2Body* body;
    sf::CircleShape shape;
    bool halved; // Flag to track if the circle has already halved in size
};

Box createBox(float x, float y, float width, float height, float density, float friction, sf::Color color)
{
    // Body definition
    b2BodyDef boxBodyDef;
    boxBodyDef.position.Set(x / PPM, y / PPM);
    boxBodyDef.type = b2_dynamicBody;

    // Shape definition
    b2PolygonShape boxShape;
    boxShape.SetAsBox(width / 2 / PPM, height / 2 / PPM);

    // Fixture definition
    b2FixtureDef fixtureDef;
    fixtureDef.density = density;
    fixtureDef.friction = friction;
    fixtureDef.shape = &boxShape;

    // Now we have a body for our Box object
    b2Body* boxBody = world.CreateBody(&boxBodyDef);
    // Lastly, assign the fixture
    boxBody->CreateFixture(&fixtureDef);

    return Box{ width, height, color, boxBody };
}

Circle createCircle(float x, float y, float radius, float density, float friction, sf::Color color)
{
    // Body definition
    b2BodyDef circleBodyDef;
    circleBodyDef.position.Set(x / PPM, y / PPM);
    circleBodyDef.type = b2_dynamicBody;

    // Shape definition
    b2CircleShape circleShape;
    circleShape.m_radius = radius / PPM;

    // Fixture definition
    b2FixtureDef fixtureDef;
    fixtureDef.density = density;
    fixtureDef.friction = friction;
    fixtureDef.shape = &circleShape;

    // Now we have a body for our Circle object
    b2Body* circleBody = world.CreateBody(&circleBodyDef);
    // Lastly, assign the fixture
    circleBody->CreateFixture(&fixtureDef);

    sf::CircleShape shape;
    shape.setOrigin(radius, radius);
    shape.setRadius(radius);
    shape.setFillColor(color);

    return Circle{ radius, color, circleBody, shape, false };
}

Box createGround(float x, float y, float width, float height, float angle, sf::Color color)
{
    // Static body definition
    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(x / PPM, y / PPM);
    groundBodyDef.angle = angle * b2_pi / 180.0f;

    // Shape definition
    b2PolygonShape groundBox;
    groundBox.SetAsBox(width / 2 / PPM, height / 2 / PPM);

    // Now we have a body for our Box object
    b2Body* groundBody = world.CreateBody(&groundBodyDef);
    // For a static body, we don't need a custom fixture definition, this will do:
    groundBody->CreateFixture(&groundBox, 0.0f);

    return Box{ width, height, color, groundBody };
}

void render(sf::RenderWindow& w, std::vector<Box>& boxes, std::vector<Circle>& circles)
{
    w.clear();

    for (const auto& box : boxes) {
        sf::RectangleShape rect;

        // For the correct Y coordinate of our drawable rect, we must subtract from WINDOW_HEIGHT
        // because SFML uses an OpenGL coordinate system where X is right, Y is down
        // while Box2D uses a traditional coordinate system where X is right, Y is up
        rect.setPosition(box.body->GetPosition().x * PPM, WINDOW_HEIGHT - (box.body->GetPosition().y * PPM));

        // We also need to set our drawable's origin to its center
        // because in SFML, "position" refers to the upper left corner
        // while in Box2D, "position" refers to the body's center
        rect.setOrigin(box.width / 2, box.height / 2);

        rect.setSize(sf::Vector2f(box.width, box.height));

        // For the rect to be rotated in the correct direction, we have to multiply by -1
        rect.setRotation(-1 * box.body->GetAngle() * DEG_PER_RAD);

        rect.setFillColor(box.color);
        w.draw(rect);
    }

    for (const auto& circle : circles) {
        sf::CircleShape shape = circle.shape;

        // For the correct Y coordinate of our drawable circle, we must subtract from WINDOW_HEIGHT
        // because SFML uses an OpenGL coordinate system where X is right, Y is down
        // while Box2D uses a traditional coordinate system where X is right, Y is up
        shape.setPosition(circle.body->GetPosition().x * PPM, WINDOW_HEIGHT - (circle.body->GetPosition().y * PPM));

        w.draw(shape);
    }

    // Create an SFML font
    sf::Font font;
    if (!font.loadFromFile("style/Roboto-Medium.ttf")) {
        // return -1; // Handle font loading error
    }

    // Create an SFML text object
    sf::Text text;
    popupText.setFont(font); // Set the font
    popupText.setString("Racking up a score up on a glass screen while you're withering your worth away"); // Set the text string
    popupText.setCharacterSize(48); // Set the character size
    popupText.setFillColor(sf::Color::Black); // Set the text color
    popupText.setOutlineThickness(20);
    popupText.setOutlineColor(sf::Color::White);
    // Calculate the position to center the text
    sf::FloatRect textBounds = popupText.getLocalBounds();
    popupText.setOrigin(textBounds.width / 2.0f, textBounds.height / 2.0f);
    popupText.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);

    //debug text
    debugText.setFont(font); // Set the font
    // char countString << buttonPressCount
    debugText.setString(std::to_string(buttonPressCount)); // Set the text string
    debugText.setCharacterSize(48); // Set the character size
    debugText.setFillColor(sf::Color::Black); // Set the text color
    debugText.setOutlineThickness(20);
    debugText.setOutlineColor(sf::Color::White);
    // Calculate the position to center the text
    sf::FloatRect debugTextBounds = debugText.getLocalBounds();
    debugText.setOrigin(debugTextBounds.width / 2.0f, debugTextBounds.height / 2.0f);
    debugText.setPosition(WINDOW_WIDTH / 9.0f, WINDOW_HEIGHT / 9.0f);

    w.draw(debugText);
    if (isPopupActive) {
        w.draw(popupText);
        if (popupClock.getElapsedTime().asSeconds() >= 5) {
            isPopupActive = false;
        }
    }
    if (startCounting) {
        if (popupClock.getElapsedTime().asSeconds() >= 5) {
            startCounting = false;
            buttonPressCount = 0;
        }
    }
    w.display();
}

int main()
{

    // Setup SFML window
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8.0;
    sf::RenderWindow w(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML + Box2D", sf::Style::Fullscreen, settings);
    w.setFramerateLimit(60);

    // Containers to hold all the boxes and circles we create
    std::vector<Box> boxes;
    std::vector<Circle> circles;

    // Generate ground
    //    boxes.push_back(createGround(0.0f, 0.0f, 3850.0f, 5.0f, 0.0f, sf::Color::White));
    boxes.push_back(createGround(0.0f, 0.0f, 5.0f, 3850.0f, 0.0f, sf::Color::White)); //left
    boxes.push_back(createGround(1920.0f, 0.0f, 5.0f, 3850.0f, 0.0f, sf::Color::White)); //right

    //K

    int red[3] = { 255, 105, 97 };
    int green[3] = { 193, 255, 193 };
    int blue[3] = { 174, 198, 207 };

    boxes.push_back(createGround(500.0f, 1025.f, 250.0f, 75.0f, 25.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(175.0f, 955.f, 300.0f, 50.0f, -25.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(225.f, 865.f, 350.0f, 50.0f, -5.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(425.0f, 725.f, 100.0f, 100.0f, 5.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(250.0f, 725.f, 125.0f, 50.0f, -25.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(250.0f, 500.f, 125.0f, 50.0f, -25.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(450.0f, 350.f, 85.0f, 170.0f, 25.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(250.0f, 275.f, 100.0f, 200.0f, -25.0f, sf::Color(green[0], green[1], green[2])));

    boxes.push_back(createGround(520.0f, 790.f, 600.0f, 50.0f, 45.0f, sf::Color(225, 165, 0))); //k
    boxes.push_back(createGround(520.0f, 400.f, 600.0f, 50.0f, -45.0f, sf::Color(225, 165, 0))); //k
    boxes.push_back(createGround(220.0f, 600.f, 50.0f, 900.0f, 0.0f, sf::Color(225, 165, 0))); //k
    boxes.push_back(createGround(125.0f, 700.f, 100.0f, 40.0f, -50.0f, sf::Color(225, 165, 0))); //topspark
    boxes.push_back(createGround(100.0f, 575.f, 100.0f, 40.0f, 0.0f, sf::Color(225, 165, 0))); //middle spark
    boxes.push_back(createGround(125.0f, 450.f, 100.0f, 40.0f, 50.0f, sf::Color(225, 165, 0))); //bottomspark

    //I
    boxes.push_back(createGround(1000.0f, 875.f, 50.0f, 200.0f, 0.0f, sf::Color(225, 165, 0))); //I
    boxes.push_back(createGround(1000.0f, 425.f, 50.0f, 400.0f, 0.0f, sf::Color(225, 165, 0))); //I

    boxes.push_back(createGround(1000.0f, 975.f, 400.0f, 50.0f, -10.0f, sf::Color(225, 165, 0))); //I
    boxes.push_back(createGround(1000.0f, 150.f, 400.0f, 50.0f, -10.0f, sf::Color(225, 165, 0))); //I

    boxes.push_back(createGround(1285.0f, 1000.f, 100.0f, 75.0f, 20.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(1250.0f, 800.f, 250.0f, 60.0f, 15.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(905.0f, 750.f, 250.0f, 60.0f, 15.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(1000.0f, 575.f, 100.0f, 100.0f, -60.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(825.0f, 500.f, 100.0f, 100.0f, -15.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(650.0f, 500.f, 150.0f, 50.0f, 15.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(625.0f, 300.f, 75.0f, 50.0f, 5.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(725.0f, 175.f, 60.0f, 150.0f, -5.0f, sf::Color(red[0], red[1], red[2])));

    boxes.push_back(createGround(1525.0f, 975.f, 175.0f, 125.0f, 20.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(1350.0f, 875.f, 100.0f, 50.0f, -30.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(1500.0f, 825.f, 100.0f, 50.0f, 30.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(1325.0f, 725.f, 150.0f, 100.0f, -30.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(1450.0f, 675.f, 100.0f, 50.0f, -5.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(1525.0f, 600.f, 100.0f, 125.0f, -5.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(1550.0f, 450.f, 200.0f, 50.0f, 10.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(1250.0f, 525.f, 300.0f, 70.0f, 10.0f, sf::Color(blue[0], blue[1], blue[2])));
    boxes.push_back(createGround(1275.0f, 350.f, 275.0f, 50.0f, -10.0f, sf::Color(green[0], green[1], green[2])));

    boxes.push_back(createGround(1325.0f, 550.f, 50.0f, 850.0f, 2.0f, sf::Color(225, 165, 0))); //D
    boxes.push_back(createGround(1575.0f, 800.f, 50.0f, 475.0f, 45.0f, sf::Color(225, 165, 0))); //D
    boxes.push_back(createGround(1575.0f, 400.f, 50.0f, 550.0f, -30.0f, sf::Color(225, 165, 0))); //D

    //extra
    boxes.push_back(createGround(1825.0f, 750.f, 150.0f, 75.0f, 25.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(1750.0f, 550.f, 150.0f, 75.0f, -25.0f, sf::Color(blue[0], blue[1], blue[2])));

    boxes.push_back(createGround(1800.0f, 350.f, 200.0f, 50.0f, 25.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(1700.0f, 200.f, 150.0f, 75.0f, -25.0f, sf::Color(red[0], red[1], red[2])));
    boxes.push_back(createGround(600.0f, 700.f, 150.0f, 75.0f, -25.0f, sf::Color(green[0], green[1], green[2])));
    boxes.push_back(createGround(800.0f, 850.f, 150.0f, 75.0f, -15.0f, sf::Color(blue[0], blue[1], blue[2])));

    // Create a ball
    // auto&& circle = createCircle(500, WINDOW_HEIGHT * 0.9f, 12, 1.f, 0.7f, sf::Color::White);
    // circles.push_back(circle);

    // Create a shrinker (yellow box)
    auto&& shrinker = createGround(0.0f, 0.0f, 3850.0f, 5.0f, 0.0f, sf::Color::Yellow);
    boxes.push_back(shrinker);

    // Create a pink box below the yellow box
    // auto&& pinkBox = createBox(800, WINDOW_HEIGHT * 0.6f, 150, 5, 1.f, 0.7f, sf::Color(255, 105, 180));
    // boxes.push_back(pinkBox);

    // Keypress stopper
    bool rl = false;
    bool gl = false;
    bool bl = false;

    /** GAME LOOP **/
    while (w.isOpen()) {
        sf::Event event;
        while (w.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                w.close();
        }

        // Update the world, standard arguments
        world.Step(1 / 60.f, 6, 3);
        // Render everything
        render(w, boxes, circles);

        // Keypresses
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {

            if (rl == false) {
                if (!isPopupActive) {
                    buttonPressCount++;
                    if (buttonPressCount >= 10) {
                        isPopupActive = true;
                        buttonPressCount = 0;
                        popupClock.restart();
                    }
                }
                if (isPopupActive == false) {
                    auto&& circle = createCircle(500, WINDOW_HEIGHT * 1.02f, 12, 5.f, 0.1f, sf::Color::Red);
                    circles.push_back(circle);
                    rl = true;
                }
            }
        }
        else {
            rl = false;
        }

        // Green
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {

            if (gl == false) {
                if (!isPopupActive) {
                    buttonPressCount++;
                    if (buttonPressCount >= 10) {
                        isPopupActive = true;
                        buttonPressCount = 0;
                        popupClock.restart();
                    }
                }
                if (isPopupActive == false) {
                    auto&& circle = createCircle(900, WINDOW_HEIGHT * 1.02f, 12, 5.f, 0.1f, sf::Color::Green);
                    circles.push_back(circle);
                    gl = true;
                }
            }
        }
        else {
            gl = false;
        }

        // Blue
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) {

            if (bl == false) {
                if (!isPopupActive) {
                    buttonPressCount++;
                    if (buttonPressCount >= 10) {
                        isPopupActive = true;
                        buttonPressCount = 0;
                        popupClock.restart();
                    }
                }
                if (isPopupActive == false) {
                    auto&& circle = createCircle(1500, WINDOW_HEIGHT * 1.02f, 12, 5.f, 0.1f, sf::Color::Blue);
                    circles.push_back(circle);
                    bl = true;
                }
            }
        }
        else {
            bl = false;
        }

        if (buttonPressCount != lastButtonPressCount) { // when the button is starting to be pressed, we need to reset it if the person stops pressing
            startCounting = true;
            popupClock.restart();
        }

        // Check for collision between balls and yellow box
        for (auto& circle : circles) {
            bool collided = false;
            for (b2ContactEdge* edge = circle.body->GetContactList(); edge; edge = edge->next) {
                if (edge->other == shrinker.body && !circle.halved) {
                    collided = true;
                    break;
                }
            }

            if (collided) {
                // Reduce the size of the ball
                b2Fixture* fixture = circle.body->GetFixtureList();
                b2Shape* shape = fixture->GetShape();
                b2CircleShape* circleShape = dynamic_cast<b2CircleShape*>(shape);
                if (circleShape) {
                    // Get the current radius
                    float currentRadius = circleShape->m_radius;
                    // Halve the radius
                    float newRadius = currentRadius / 2;
                    // Update the shape
                    circleShape->m_radius = newRadius;

                    // Update the visual size of the circle
                    circle.shape.setRadius(newRadius * PPM);
                    circle.shape.setOrigin(newRadius * PPM, newRadius * PPM);

                    circle.halved = true; // Set the flag to indicate halving has occurred
                }
            }
        }
        lastButtonPressCount = buttonPressCount;
    }

    return 0;
}