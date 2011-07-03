#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>
#include <utility>

class Surface;
class Screen;
class Position;
class Room;
class Layer;

class Position
{
  public:
    Position(int t_x, int t_y)
      : m_x(t_x), m_y(t_y)
    {
    }

    const int &x() const
    {
      return m_x;
    }

    const int &y() const
    {
      return m_y;
    }

    int &x() 
    {
      return m_x;
    }

    int &y()
    {
      return m_y;
    }

    Position operator+(const Position &t_rhs) const
    {
      return Position(m_x + t_rhs.m_x, m_y + t_rhs.m_y);
    }

    bool operator<(const Position &t_rhs) const
    {
      return (m_y < t_rhs.m_y)
        || (m_y == t_rhs.m_y && m_x < t_rhs.m_x);
    }

  private:
    int m_x;
    int m_y;

};

class Surface
{
  public:
    typedef char* (*ErrorFunc)();

    Surface(SDL_Surface *t_surf)
      : m_surface(t_surf)
    {
      if (!m_surface)
      {
        throw std::runtime_error(SDL_GetError());
      }
    }

    Surface(SDL_Surface *t_surf, ErrorFunc t_errfunc)
      : m_surface(t_surf)
    {
      if (!m_surface)
      {
        throw std::runtime_error(t_errfunc());
      }
    }

    void clear()
    {
      SDL_Rect dest;
      dest.x=0;
      dest.y=0;
      dest.w=m_surface->w;
      dest.h=m_surface->h;
      SDL_FillRect(m_surface, &dest, 0);
    }

    void flip()
    {
      SDL_Flip(m_surface);
    }

    ~Surface()
    {
      SDL_FreeSurface(m_surface);
    }

    void render(Surface &t_surface, const Position &t_position) const
    {
      SDL_Rect dest;
      dest.x = t_position.x();
      dest.y = t_position.y();
      dest.w = m_surface->w;
      dest.h = m_surface->h;

      SDL_BlitSurface(m_surface, NULL, t_surface.m_surface, &dest);
    }

    int width() const
    {
      return m_surface->w;
    }

    int height() const
    {
      return m_surface->h;
    }

  private:
    SDL_Surface *m_surface;
};

class Screen
{
  public:
    Screen()
      : m_initializer(), m_surface(SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE))
    {

    }


    Surface &getSurface()
    {
      return m_surface;
    }

  private:
    struct Initializer
    {
      Initializer()
      {
        if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0 ) {
          throw std::runtime_error(std::string("Unable to init SDL: ") + SDL_GetError());
        }
      }

      ~Initializer()
      {
        SDL_Quit();
      }
    };

    Initializer m_initializer;
    Surface m_surface;
};

class Object
{
  public:
    Object(const std::string &t_filename)
      : m_surface(IMG_Load(t_filename.c_str()), &IMG_GetError)
    {
    }

    ~Object()
    {
    }

    void render(Surface &t_surface, Position t_position) const
    {
      m_surface.render(t_surface, t_position);
    }


  private:
    Object(const Object &);
    Object &operator=(const Object &);

    Surface m_surface;
};

class Layer
{
  public:
    Layer(int t_width, int t_height)
      : m_width(t_width), m_height(t_height)
    {
    }

    void addObject(Position t_p, const boost::shared_ptr<Object> &t_obj)
    {
      m_objects.insert(std::make_pair(t_p, t_obj));
    }

    void render(Surface &t_surface, const Position &t_offset) const
    {
      for (std::set<std::pair<Position, boost::shared_ptr<Object> > >::const_iterator itr = m_objects.begin();
           itr != m_objects.end();
           ++itr)
      {

        itr->second->render(t_surface, itr->first + t_offset);
      }
    }

    int width() const
    {
      return m_width;
    }

    int height() const
    {
      return m_height;
    }


  private:
    int m_width;
    int m_height;

    std::set<std::pair<Position, boost::shared_ptr<Object> > > m_objects;
};


class Room
{
  public:

    void addLayer(const boost::shared_ptr<Layer> &t_layer)
    {
      m_layers.push_back(t_layer);
    }

    void render(Surface &t_surface, const boost::shared_ptr<Layer> &t_center_layer,
        const Position &t_pos_on_layer) const
    {

      std::vector<boost::shared_ptr<Layer> >::const_iterator foundlayer = 
        std::find(m_layers.begin(), m_layers.end(), t_center_layer);

      if (foundlayer == m_layers.end())
      {
        throw std::runtime_error("Requested center layer doesn't exist in room");
      }

      double xpercent = double(t_pos_on_layer.x()) / (*foundlayer)->width();
      double ypercent = double(t_pos_on_layer.y()) / (*foundlayer)->height();

      int renderwidth = t_surface.width();
      int renderheight = t_surface.height();

      for (std::vector<boost::shared_ptr<Layer> >::const_iterator itr = m_layers.begin();
           itr != m_layers.end();
           ++itr)
      {
        int xcenter = (*itr)->width() * xpercent;
        int ycenter = (*itr)->height() * ypercent;

        int xoffset = -xcenter + renderwidth / 2;
        int yoffset = -ycenter + renderheight / 2;

        (*itr)->render(t_surface, Position(xoffset, yoffset));
      }

      t_surface.flip();
    }

  private:
    std::vector<boost::shared_ptr<Layer> > m_layers;

};


struct State
{
  Position p;

  State()
    : p(100, 100)
  {
  }
};

void handleKeyPress(State &t_state, SDLKey t_key)
{
  switch (t_key)
  {
    case SDLK_LEFT:
      t_state.p.x() -= 10;
      break;
    case SDLK_RIGHT:
      t_state.p.x() += 10;
      break;
    case SDLK_UP:
      t_state.p.y() -= 10;
      break;
    case SDLK_DOWN:
      t_state.p.y() += 10;
      break;

    default:

     ;
  }
}


int main()
{
  Screen s;

  State state;

  Room r1;
  boost::shared_ptr<Layer> clouds(new Layer(640, 480));
  boost::shared_ptr<Layer> play(new Layer(1000, 1000));
  boost::shared_ptr<Object> o1(new Object("cloud.png"));
  boost::shared_ptr<Object> o2(new Object("tree.png"));
  r1.addLayer(play);
  play->addObject(Position(45, 100), o2);
  play->addObject(Position(60, 300), o2);
  play->addObject(Position(600, 800), o2);
  play->addObject(Position(200, 800), o2);

  r1.addLayer(clouds);
  clouds->addObject(Position(10, 10), o1);
  clouds->addObject(Position(100, 10), o1);
  clouds->addObject(Position(300, 10), o1);
  clouds->addObject(Position(10, 400), o1);


  while (true)
  {
    SDL_Event e;
    if (SDL_PollEvent(&e))
    {
      switch (e.type)
      {
        case SDL_KEYDOWN:
          handleKeyPress(state, e.key.keysym.sym);
          s.getSurface().clear();
          r1.render(s.getSurface(), play, state.p);
          break;
        default:
          ;
      }


    }
  }

  char c;
  std::cin >> c;
}


