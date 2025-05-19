#pragma once

#include <JuceHeader.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// juce stuff
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace juce_stuff {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// juce stuff :: concepts
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace concepts {

template < class T, class U >
concept derived = std::is_base_of< U, T >::value;

} //namespace concepts


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// juce stuff :: components
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace components {

//---------------------------------------------------------------------------------------------------------------------------
// juce stuff :: components :: with sub components
//---------------------------------------------------------------------------------------------------------------------------

using component = juce::Component;
using colour    = juce::Colour;

struct has_sub_components { };

struct factory {

    template < typename T, class... Args >
    static component* build( Args... args ) {
        return new T( args... ); 
    };

    template < typename T, typename U >
    static component* build( std::initializer_list< U > args ) {
        return new T( args );
    };


    template < typename T, class... Args >
    static component* build_and_lambda( auto lambda, Args... args ) {
        T* object = new T( args... );
        lambda( *object );

        return object;
    };

    template < typename T, typename U >
    static component* build_and_lambda( auto lambda, std::initializer_list< U > args ) {
        T* object = new T( args );
        lambda( *object );

        return object;
    };

};

template < concepts::derived< component > T = component >
struct with_sub_components : public T, public has_sub_components {
public:
    using parent_type = T;
    using type        = with_sub_components< T >;

protected:
    juce::OwnedArray< component > sub_components;

public:
    with_sub_components( std::initializer_list< component* > components ) {
        std::for_each(
            components.begin(),
            components.end(),
            [ & ] ( component* the_component ) {
                this->add_sub_component( the_component );
            }
        );
    }

    template < typename T >
    T* add_sub_component( T* the_component ) {
        sub_components.add( the_component );
        this->addAndMakeVisible( the_component );
        return the_component;
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// juce stuff :: components :: with hint label
//---------------------------------------------------------------------------------------------------------------------------

template < const char* _name, const char* _hint, concepts::derived< component > T = component >
struct with_hint_label : public T {
public:
    static inline const char* name         = _name;
    static inline const char* hint         = _hint;
    using                     parent_type  = T;
    using                     type         = with_hint_label< name, hint, T >;

    juce::Label hint_label{ name, hint };

    with_hint_label( void ) {
        hint_label.set_bounds( this->getBounds( ) );
        this->addAndMakeVisible( hint_label );
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// juce stuff :: components :: with_tabs
//---------------------------------------------------------------------------------------------------------------------------

template < juce::TabbedButtonBar::Orientation _orientation, concepts::derived< juce::TabbedComponent > T = juce::TabbedComponent >
struct with_tabs : public T {
    struct initializer {
        using lambda = std::function< void( juce::TabBarButton* ) >;

        initializer(
            const char*             _the_label,
            const colour            _the_colour,
            component*              _the_component,
            const bool              _delete_when_not_needed,
            std::optional< lambda > _tab_button_initializer = std::nullopt ) :
                the_label( _the_label ),
                the_colour( _the_colour ),
                the_component( _the_component ),
                delete_when_not_needed( _delete_when_not_needed ),
                tab_button_initializer( _tab_button_initializer ) {
        }

        const char*                 the_label;
        const colour                the_colour;
        component*                  the_component;
        const bool                  delete_when_not_needed;
        std::optional< lambda >     tab_button_initializer;
    };

    using                             orientation_t         = juce::TabbedButtonBar::Orientation;
    static inline const orientation_t orientation           = _orientation;
    using                             parent_type           = T;
    using                             type                  = with_tabs< orientation, parent_type >;
    using                             initializer_list      = std::initializer_list< initializer >;

    with_tabs( initializer_list tabs )
        : parent_type( orientation ) {
        int index = 0;
        std::for_each(
            tabs.begin( ),
            tabs.end( ),
            [ & ]( initializer tab ) {
                this->addTab( tab.the_label, tab.the_colour, tab.the_component, tab.delete_when_not_needed );
                if ( std::nullopt != tab.tab_button_initializer ) {
                    ( *tab.tab_button_initializer )( this->getTabbedButtonBar( ).getTabButton( index ) );
                }
                ++index;
            }
        );

        
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// juce stuff :: components :: with grid
//---------------------------------------------------------------------------------------------------------------------------

template < concepts::derived< juce::Component > T = juce::Component >
struct with_grid : public with_sub_components< T > {
    using                             parent_type       = with_sub_components< T >;
    using                             type              = with_grid< parent_type >;
    using                             initializer_list  = std::initializer_list< component* >;

    juce::Grid          grid;

    with_grid( initializer_list grid_items )
        : parent_type( grid_items ) {
        
        std::for_each(
            grid_items.begin( ),
            grid_items.end( ),
            [ & ] ( component* grid_item ) {
                grid.items.add( juce::GridItem( grid_item ) );
            }
        );
    }

    void resized( ) override {
        grid.performLayout( this->getLocalBounds( ) );
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// juce stuff :: components :: button
//---------------------------------------------------------------------------------------------------------------------------

//template < juce::TabbedButtonBar::Orientation _orientation, concepts::derived< juce::TabbedComponent > T = juce::TabbedComponent >
//struct button : public T {
//{
//    button(){
//        juce::ToggleButton nativeButton;
//        addAndMakeVisible( nativeButton );
//        nativeButton.setButtonText( "Use Native Windows" );
//        nativeButton.onClick = [ this ] { getLookAndFeel( ).setUsingNativeAlertWindows( nativeButton.getToggleState( ) ); };
//    }
//};

} // namespace components

} // namespace juce_stuff