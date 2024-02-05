# jdbgx 

Little C++ Java native agent.

For now it only supports printing out primitive type variable values to stdout.

Building the shared lib:
```
$ ./build.sh
```

Attaching the live agent:
```
$ java -agentpath:./agent.so -Xdebug Source
```

This example Java program with a breakpoint set on line 4 and 12 produces the following ouput

```java
public class Source {
    public static void example() {
      String var_string = "e";
      int var_integer = 0;
    }

    public static int random_method() {
      int value = 54;
      int value2 = 54;
      float float_value = 54f;
      double double_value = 54D;
      return value;
    }

    public static void main(String[] args) {
      example();
      random_method();
    }
}
```

```
JDBG AGENT: Loading
Loaded breakpoint list successfuly
thread: main
thread: Reference Handler
thread: Finalizer
thread: Signal Dispatcher
JDBG AGENT: VMInit event received
Setting breakpoint at: random_method line: 12
Setting breakpoint at: example line: 4
[debug_breakpoint] breakpoint hit
[debug_loc_var] var_integer = 1
[debug_breakpoint] breakpoint hit
[debug_loc_var] value = 54
[debug_loc_var] value2 = 54
[debug_loc_var] float_value = 54
[debug_loc_var] double_value = 54
JDBG AGENT: Unloaded
```