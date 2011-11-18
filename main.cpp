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
    Position(double t_x, double t_y)
      : m_x(t_x), m_y(t_y)
    {
    }

    const double &x() const
    {
      return m_x;
    }

    const double &y() const
    {
      return m_y;
    }

    double &x() 
    {
      return m_x;
    }

    double &y()
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
    double m_x;
    double m_y;

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
//      SDL_SetAlpha(m_surface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
      SDL_FillRect(m_surface, &dest, SDL_MapRGBA(m_surface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));
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

    double width() const
    {
      return m_surface->w;
    }

    double height() const
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
      : m_initializer(), m_surface(SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_HWACCEL))
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
        if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0 ) {
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
    Layer(const std::string &t_image)
      : m_dirty(false), m_surface(IMG_Load(t_image.c_str()), &IMG_GetError),
        m_rendered_surface(IMG_Load(t_image.c_str()), &IMG_GetError)
    {
    }

    void addObject(Position t_p, const boost::shared_ptr<Object> &t_obj)
    {
      m_objects.insert(std::make_pair(t_p, t_obj));
      m_dirty = true;
    }

    void render(Surface &t_surface, const Position &t_offset) const
    {
      if (m_dirty) 
      {
//        m_rendered_surface.clear();
//        m_surface.render(m_rendered_surface, Position(0, 0));
 
        for (std::set<std::pair<Position, boost::shared_ptr<Object> > >::const_iterator itr = m_objects.begin();
             itr != m_objects.end();
             ++itr)
        {
          itr->second->render(m_rendered_surface, itr->first);
        }

        m_dirty = false;
      }

      m_rendered_surface.render(t_surface, t_offset);
    }

    double width() const
    {
      return m_surface.width();
    }

    double height() const
    {
      return m_surface.height();
    }


  private:
    mutable bool m_dirty;

    Surface m_surface; // cached surface

    mutable Surface m_rendered_surface; // cached surface

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

      double renderwidth = t_surface.width();
      double renderheight = t_surface.height();

      for (std::vector<boost::shared_ptr<Layer> >::const_iterator itr = m_layers.begin();
           itr != m_layers.end();
           ++itr)
      {
        double xcenter = (*itr)->width() * xpercent;
        double ycenter = (*itr)->height() * ypercent;

        double xoffset = -xcenter + renderwidth / 2;
        double yoffset = -ycenter + renderheight / 2;

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

  bool moving_left;
  bool moving_right;
  bool moving_up;
  bool moving_down;

  double s_per_frame;

  int frame_count;


  State()
    : p(100, 100),
      moving_left(false),
      moving_right(false),
      moving_up(false),
      moving_down(false),
      s_per_frame(.03),
      frame_count(0)
  {
  }
};

void handleKey(State &t_state, SDL_KeyboardEvent t_key)
{
  bool keypressed = t_key.state == SDL_PRESSED;

  switch (t_key.keysym.sym)
  {
    case SDLK_LEFT:
      t_state.moving_left = keypressed;
      break;
    case SDLK_RIGHT:
      t_state.moving_right = keypressed;
      break;
    case SDLK_UP:
      t_state.moving_up = keypressed;
      break;
    case SDLK_DOWN:
      t_state.moving_down = keypressed;
      break;

    default:

     ;
  }
}

void updateState(State &t_state)
{
  if (t_state.moving_left)
  {
    t_state.p.x() -= 50 * t_state.s_per_frame;
  }
  if (t_state.moving_right)
  {
    t_state.p.x() += 50 * t_state.s_per_frame;
  }
  if (t_state.moving_up)
  {
    t_state.p.y() -= 50 * t_state.s_per_frame;
  }
  if (t_state.moving_down)
  {
    t_state.p.y() += 50 * t_state.s_per_frame;
  }
}

struct Quit_Exception : std::runtime_error
{
  Quit_Exception() throw()
    : std::runtime_error("Quit Requested")
  {
  }

  virtual ~Quit_Exception() throw()
  {
  }
};

void handleSDLEvents(State &t_state)
{
  SDL_Event e;
  if (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        handleKey(t_state, e.key);
        break;
      case SDL_QUIT:
        throw Quit_Exception();
        break;
      case SDL_USEREVENT:
        t_state.s_per_frame = 1 / (double(t_state.frame_count) * 10);
        std::cout << "FPS: " << t_state.frame_count * 10 << std::endl << std::flush;
        t_state.frame_count = 0;
        break;

      default:
        ;
    }
  }
}

uint32_t timerevent(uint32_t interval, void *)
{
  SDL_Event event;
  event.type = SDL_USEREVENT;
  SDL_PushEvent(&event);

  return interval;

}

int main()
{
  Screen s;

  State state;

  Room r1;
  boost::shared_ptr<Layer> clouds(new Layer("clouds.png"));
  boost::shared_ptr<Layer> play(new Layer("play.png"));
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

  bool cont = true;
  SDL_AddTimer(100, &timerevent, 0);

  while (cont)
  {
    try {
      handleSDLEvents(state);
      updateState(state);
    } catch (const Quit_Exception &) {
      cont = false;
    }

    s.getSurface().clear();
    r1.render(s.getSurface(), play, state.p);

    ++state.frame_count;
  }

}


