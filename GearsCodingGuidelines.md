# Introduction #

This page documents some guidelines you should follow when writing Gears code.


# HTML UI Files #

All strings used in .html files are stored in their associated .js.stab file.  Strings should be given an id matching the id of the div or span they will appear in.

The **TRANS\_BLOCK** tag is used inside the .stab files to mark strings for translation.

  * Please try not to change the text in these blocks if not absolutely necessary as this requires sending them for retranslation.
  * Do not put HTML tags inside of 

<TRANS\_BLOCK>

 tags if it can be avoided.  This adds clutter to our strings, and adds a point of failure for the UIs in localized versions of Gears.

# Reference Counted Classes #

Gears code utilizes intrusive reference counting on some objects via `Ref()` and `Unref()`.  An external reference-counting mechanism like shared\_ptr is not feasible because we must occasionally pass raw pointers via COM or other API boundaries.  To keep things consistent and minimize the number of bugs, the following guidelines should be used:

  * Use `T*` for in parameters and `scoped_refptr<T>*` for out parameters:
```
   void Func(Foo *in_param, scoped_refptr<Foo>* out_param);
```

  * Do not pass scoped\_refptr by value, as it causes unnecessary increments and decrements.  The only possible exception would be as a return value from a function, but an out parameter should be preferred.

  * If scoped\_refptr cannot be used for an out parameters, `Ref()` the pointer before returning it, and remember to `Unref()` the returned pointer after you're done with it:
```
   void GetFoo(Foo** f) {
      *f = new Foo;
      f->Ref();
   }
   Foo* local_raw;
   GetFoo(&local);
   scoped_refptr<Foo> local(local_raw);
   local_raw->Unref();
```
> Since this is so ugly, consider using the as\_out\_parameter idiom:
```
   void GetFoo(Foo** f);
   scoped_refptr<Foo> local;
   GetFoo(as_out_parameter(local));
```

# Mac coding guidelines #

  * Try to minimize the use of Objective-C - Objective-C can't be utilized in other platforms, the more common code we have in Gears the better.
  * Use Cocoa for UI - The Carbon UI elements are deprecated and supposedly won't make the 64-bit transition.  Use Cocoa instead.

## Notable Departures from common Objective-C coding style ##
  * In .mm (obj-c++) files, use #import (rather than #include) for both obj-c and pure c++ header files.
  * Add #define guards to obj-c headers files.
  * In contrast to common Objective-C style, long variable names follow the same conventions as C++ rather than the traditional Apple camelCase convention, e.g.
```
// Don't:
@interface Foo : NSObject {
  NSString *someStringVariable_;
}
@end

// Do:
@interface Foo : NSObject {
  NSString *some_string_variable_;
}
@end
```