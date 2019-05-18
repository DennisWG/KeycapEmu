module keycap.shared.knowledge_base;

data category
{
    [primary][not_null][increment]
    int32 id;
    
    [not_null]
    string name;
}

[keys="id, category"]
data sub_category
{
    [not_null][increment]
    int32 id;

    [not_null][foreign_key="category(id)"][on_update="CASCADE"][on_delete="CASCADE"]
    int32 category;

    [not_null]
    string name;
}

data article
{
    [primary][not_null][increment]
    int32 id;

    [not_null][foreign_key="category(id)"][on_update="CASCADE"][on_delete="CASCADE"]
    int32 category;

    [not_null][foreign_key="sub_category(id)"][on_update="CASCADE"][on_delete="CASCADE"]
    int32 sub_category;

    [not_null][fulltext]
    string subject;

    [not_null]
    bool is_hot;

    [not_null]
    bool is_updated;

    [not_null][fulltext]
    string text;
}