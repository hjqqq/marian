/* -*- c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "Game.h"

#include "ResourceMgr.h"
#include "Engine.h"
#include "Actor.h"
#include "ActorFactory.h"
#include "Util.h"
#include "ScriptMgr.h"

#include <iostream>

Game::Game()
{
    b2Vec2 gravity(0.0f, -9.8f);
    m_world = new b2World(gravity);
    m_world->SetAllowSleeping(true);
    m_world->SetContactListener(&m_contactListener);
    
    // Debug drawing
    m_debugDraw = new DebugDraw;
    m_drawDebugData = false;
    
    // Read map from file
    ResourceMgr& resMgr = ResourceMgr::singleton();
    resMgr.setDataFolder("media/");

    // Hero texture
    resMgr.addTexture("MegaMan_001.png");

    m_map.loadFromFile("media/map2.tmx");

    // Build objects
    std::vector<MapObject> mapObjects;
    m_map.getObjects(mapObjects);

    std::cout << "INFO: " << mapObjects.size() << " objects loaded.\n";

    for (const MapObject& obj : mapObjects) {
        m_gameObjects.push_back(ActorFactory::create(this, obj));
        if (obj.name == "hero")
            Engine::singleton().centerViewOn(obj.x, obj.y);
    }
}

//------------------------------------------------------------------------------

Game::~Game()
{
    for (Actor *go : m_gameObjects) {
        delete go;
    }

    delete m_world;
    delete m_debugDraw;
}

//------------------------------------------------------------------------------

void Game::initialize(Engine *e)
{
    // Set background color
    std::string bgColor = m_map.backgroundColor(); 
    if (!bgColor.empty()) {
        std::vector<int> color = hexColorToRgb(bgColor);
        e->setBackgroundColor(color[0], color[1], color[2]);
    }

    ScriptMgr::singleton().executeScript("map2_init.lua");
}

//------------------------------------------------------------------------------

bool Game::processInput(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_KEYUP:
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) return false;
        if (event.key.keysym.sym == SDLK_g) toggleDrawDebug();
        break;
    }
    return true; // keep going
}

//------------------------------------------------------------------------------

void Game::update(Engine *e, float elapsedTime)
{
    // Update Physics
    int32 velocityIterations = 6;
    int32 positionIterations = 2;
    m_world->Step(elapsedTime, velocityIterations, positionIterations);

    for (Actor *go : m_gameObjects) {
        go->update(e, elapsedTime);
    } 

    m_fpsCounter.update(elapsedTime);

    // Remove dead GameObjects
    auto it = std::begin(m_gameObjects);
    while (it != std::end(m_gameObjects)) {
        Actor *go = *it;
        if (go->dead()) {
            delete go;
            it = m_gameObjects.erase(it);
        } else {
            // Kill it if out of map
            if (!isOnMap(go))
                go->die();
            ++it;
        }
    }
}

//------------------------------------------------------------------------------

void Game::draw(Engine *e)
{
    // Draw only visible part of the map
    float x1, x2, y1, y2;
    e->viewBounds(&x1, &x2, &y1, &y2);

    // TODO: read order from map
    m_map.drawLayer(e, "back",   x1, x2, y1, y2);
    m_map.drawLayer(e, "ground", x1, x2, y1, y2);
 
    for (Actor* go : m_gameObjects) {
        go->draw(e);
    }

    m_map.drawLayer(e, "water", x1, x2, y1, y2);
    m_map.drawLayer(e, "front", x1, x2, y1, y2);

    if (m_drawDebugData) {
        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        glDisable(GL_TEXTURE_2D);
        m_world->DrawDebugData();

        glPopAttrib();
    }
}

//------------------------------------------------------------------------------

void Game::addGameObject(ActorCategory type, const std::string& name,
                         float x, float y)
{
    m_gameObjects.push_back(ActorFactory::create(this, type, name, x, y));        
}

//------------------------------------------------------------------------------

bool Game::isOnMap(Actor *go)
{
    const b2Vec2& pos = go->body()->GetPosition();
    if (pos.x < 0.0f || pos.x > m_map.width())
        return false;
    else if (pos.y < 0.0f)
        return false;

    return true;
}

//------------------------------------------------------------------------------

void Game::toggleDrawDebug()
{
    if (m_drawDebugData == false) {
        m_debugDraw->SetFlags(b2Draw::e_shapeBit | b2Draw::e_centerOfMassBit);
        m_world->SetDebugDraw(m_debugDraw);    
    } else {
        m_world->SetDebugDraw(nullptr);
    }

    m_drawDebugData = !m_drawDebugData;
}

//------------------------------------------------------------------------------

void ContactListener::BeginContact(b2Contact *contact)
{
    void *fixAUserData = contact->GetFixtureA()->GetUserData();
    void *fixBUserData = contact->GetFixtureB()->GetUserData();

    void *bodyAUserData = contact->GetFixtureA()->GetBody()->GetUserData();
    void *bodyBUserData = contact->GetFixtureB()->GetBody()->GetUserData();
    if ( bodyAUserData && bodyBUserData ) {
        Actor *gameObjectA = static_cast<Actor *>(bodyAUserData);
        Actor *gameObjectB = static_cast<Actor *>(bodyBUserData);
        gameObjectA->handleBeginContact(gameObjectB, fixAUserData);
        gameObjectB->handleBeginContact(gameObjectA, fixBUserData);
    }
}

//------------------------------------------------------------------------------

void ContactListener::EndContact(b2Contact *contact)
{
    void *fixAUserData = contact->GetFixtureA()->GetUserData();
    void *fixBUserData = contact->GetFixtureB()->GetUserData();

    void *bodyAUserData = contact->GetFixtureA()->GetBody()->GetUserData();
    void *bodyBUserData = contact->GetFixtureB()->GetBody()->GetUserData();
    if ( bodyAUserData && bodyBUserData ) {
        Actor *gameObjectA = static_cast<Actor *>(bodyAUserData);
        Actor *gameObjectB = static_cast<Actor *>(bodyBUserData);
        gameObjectA->handleEndContact(gameObjectB, fixAUserData);
        gameObjectB->handleEndContact(gameObjectA, fixBUserData);
    }
}
