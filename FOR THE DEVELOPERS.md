# Syntactic & System features
> JUCE simplifies code writing, file linking and assembly using built-in methods, macros, etc., but also requires specifics when working with audio systems.

Next, the main elements will be given to simplify not so much reading as understanding the code.

__Full documentation is__ [___here___](https://docs.juce.com/master/group__juce__core-system.html)
***
### JUCE elements
##### MACROS
* __JUCE_CALLTYPE__ - This macro defines the C calling convention used as the standard for JUCE calls.
* __JUCE_LEAK_DETECTOR__ - This macro lets you embed a leak-detecting object inside a class.
* __JUCE_DECLARE__ _ __NON_COPYABLE__ - This is a shorthand macro for deleting a class's copy constructor and copy assignment operator.
* __JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR__ - This is a shorthand way of writing both a JUCE_DECLARE_NON_COPYABLE and JUCE_LEAK_DETECTOR macro for a class.
##### Functions
* __jassert()__ - Platform-independent assertion macro. Defined as a function.

***
### Detailed information
* [Multiband Compression](https://emastered.com/blog/what-is-multiband-compression)
* [Fifth music system](https://habr.com/ru/post/653621/)
