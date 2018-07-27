package f8left.fupk3.core;
//===-----------------------------------------------------------*- xxxx -*-===//
//
//                     Created by F8LEFT on 2017/9/1.
//                   Copyright (c) 2017. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class FRefInvoke {

    public static Class getClass(ClassLoader loader, String class_name) throws Exception{
        if (loader != null) {
            try {
                return loader.loadClass(class_name);
            } catch (Exception e){
                // class not found from loader, used Class.forName instead
            }
        }
        return Class.forName(class_name);
    }

    public static Object invokeMethod(Class clazz, String method_name, Class[] pareTyple, Object obj, Object[] pareVaules) throws Exception{
        Method method = clazz.getMethod(method_name,pareTyple);
        return method.invoke(obj, pareVaules);
    }

    public static Object getFieldOjbect(Class clazz,Object obj, String filedName) throws Exception{
        Field field = clazz.getDeclaredField(filedName);
        field.setAccessible(true);
        return field.get(obj);
    }

    public static void setFieldOjbect(Class clazz, String filedName, Object obj, Object filedVaule) throws Exception{
        Field field = clazz.getDeclaredField(filedName);
        field.setAccessible(true);
        field.set(obj, filedVaule);
    }
}
