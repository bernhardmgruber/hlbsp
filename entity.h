#ifndef _ENTITY_H_
#define _ENTITY_H_

// Structure which defines a property of an entity
typedef struct _ENTITYPROPERTY
{
    char* pszName;  // String containing the name of the property
    char* pszValue; // String containing the value of the property
} ENTITYPROPERTY;

class CEntity
{
    public:
        CEntity();  // ctor
        ~CEntity(); // dtor

        void ParseProperties(const char* pszProperties); // Parses an entity of the bsp file entity lump into ENTITYPROPERTY data structurs
        const char* FindProperty(const char* pszName);   // Returns the value of a property by the given name

    private:
        ENTITYPROPERTY* properties; // Pointer to array of properties
        int nProperties;            // The number of properties
};

#endif
